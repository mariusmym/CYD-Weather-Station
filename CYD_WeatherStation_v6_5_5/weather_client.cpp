#include "weather_client.h"
#include "secrets.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>

static bool sTimeConfigured = false;

static void ensureWiFi() {
  wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
  }
}

static void ensureTimeConfigured() {
  if (sTimeConfigured) return;
  if (WiFi.status() != WL_CONNECTED) return;

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  sTimeConfigured = true;
}

static String fmtHour(time_t ts) {
  struct tm tmv;
  localtime_r(&ts, &tmv);
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &tmv);
  return String(buf);
}

static String fmtDay(time_t ts) {
  struct tm tmv;
  localtime_r(&ts, &tmv);
  char buf[8];
  strftime(buf, sizeof(buf), "%a", &tmv);
  return String(buf);
}

static String fmtStamp(time_t ts) {
  struct tm tmv;
  localtime_r(&ts, &tmv);
  char buf[24];
  strftime(buf, sizeof(buf), "%d %b %H:%M", &tmv);
  return String(buf);
}

static String readJsonString(JsonVariantConst v, const char* fallback) {
  if (v.is<const char*>()) {
    const char* s = v.as<const char*>();
    if (s && s[0] != '\0') return String(s);
  }
  return String(fallback);
}

static String fetchUrl(const String& url, int& httpCode) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(12000);
  http.setTimeout(12000);
  if (!http.begin(client, url)) {
    httpCode = -1;
    return String();
  }

  httpCode = http.GET();
  String payload = http.getString();
  http.end();
  return payload;
}

static bool parseCurrentPayload(const String& payload, WeatherData& out, long& tzOffsetSeconds, time_t& currentLocalTs) {
  DynamicJsonDocument doc(12 * 1024);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[weather] current JSON error: ");
    Serial.println(err.c_str());
    return false;
  }

  tzOffsetSeconds = doc["timezone"] | 0;
  out.tzOffsetSeconds = tzOffsetSeconds;
  auto toLocalTs = [tzOffsetSeconds](uint32_t utc) -> time_t { return (time_t)(utc + tzOffsetSeconds); };

  JsonArrayConst weather = doc["weather"].as<JsonArrayConst>();
  JsonObjectConst wx0 = weather.size() > 0 ? weather[0].as<JsonObjectConst>() : JsonObjectConst();

  out.current.description = readJsonString(wx0["description"], "--");
  out.current.icon = readJsonString(wx0["icon"], "01d");
  out.current.temp = (int)lround((double)(doc["main"]["temp"] | 0.0));
  out.current.feelsLike = (int)lround((double)(doc["main"]["feels_like"] | 0.0));
  out.current.humidity = doc["main"]["humidity"] | 0;
  out.current.windKph = (int)lround((double)(doc["wind"]["speed"] | 0.0) * 3.6);
  out.current.pressure = doc["main"]["pressure"] | 0;
  out.current.uv = -1;
  out.current.cloudPct = doc["clouds"]["all"] | 0;
  out.current.visibilityKm = (int)lround((double)(doc["visibility"] | 0) / 1000.0);
  out.current.sunrise = toLocalTs(doc["sys"]["sunrise"] | 0);
  out.current.sunset = toLocalTs(doc["sys"]["sunset"] | 0);

  currentLocalTs = toLocalTs(doc["dt"] | 0);
  out.lastUpdated = fmtStamp(currentLocalTs);
  return true;
}

struct DailyAccumulator {
  bool used = false;
  String dayLabel;
  String icon;
  int minTemp = 1000;
  int maxTemp = -1000;
  int rainPct = 0;
  int bestHourDistance = 99;
  int yday = -1;
  time_t ts = 0;
};

static bool parseForecastPayload(const String& payload, WeatherData& out, long tzOffsetSeconds) {
  DynamicJsonDocument doc(48 * 1024);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[weather] forecast JSON error: ");
    Serial.println(err.c_str());
    return false;
  }

  auto toLocalTs = [tzOffsetSeconds](uint32_t utc) -> time_t { return (time_t)(utc + tzOffsetSeconds); };

  for (int i = 0; i < HOURLY_COUNT; ++i) out.hourly[i] = HourlyWeather();
  for (int i = 0; i < DAILY_COUNT; ++i) out.daily[i] = DailyWeather();
  out.hourlyCount = 0;
  out.dailyCount = 0;

  JsonArrayConst list = doc["list"].as<JsonArrayConst>();
  if (list.isNull() || list.size() == 0) {
    Serial.println("[weather] forecast list missing");
    return false;
  }

  int hourlyFilled = 0;
  DailyAccumulator dailyAcc[DAILY_COUNT];
  int dailyFilled = 0;

  for (JsonVariantConst itemVar : list) {
    JsonObjectConst item = itemVar.as<JsonObjectConst>();
    time_t localTs = toLocalTs(item["dt"] | 0);

    struct tm tmv;
    localtime_r(&localTs, &tmv);

    JsonArrayConst weather = item["weather"].as<JsonArrayConst>();
    JsonObjectConst wx0 = weather.size() > 0 ? weather[0].as<JsonObjectConst>() : JsonObjectConst();
    String icon = readJsonString(wx0["icon"], "01d");
    int temp = (int)lround((double)(item["main"]["temp"] | 0.0));
    int tempMin = (int)lround((double)(item["main"]["temp_min"] | item["main"]["temp"] | 0.0));
    int tempMax = (int)lround((double)(item["main"]["temp_max"] | item["main"]["temp"] | 0.0));
    int rainPct = (int)lround((double)(item["pop"] | 0.0) * 100.0);

    if (hourlyFilled < HOURLY_COUNT) {
      out.hourly[hourlyFilled].timeLabel = fmtHour(localTs);
      out.hourly[hourlyFilled].icon = icon;
      out.hourly[hourlyFilled].temp = temp;
      out.hourly[hourlyFilled].rainPct = rainPct;
      hourlyFilled++;
      out.hourlyCount = hourlyFilled;
    }

    int slot = -1;
    for (int i = 0; i < dailyFilled; ++i) {
      if (dailyAcc[i].yday == tmv.tm_yday) {
        slot = i;
        break;
      }
    }
    if (slot == -1) {
      if (dailyFilled >= DAILY_COUNT) continue;
      slot = dailyFilled++;
      dailyAcc[slot].used = true;
      dailyAcc[slot].yday = tmv.tm_yday;
      dailyAcc[slot].ts = localTs;
      dailyAcc[slot].dayLabel = fmtDay(localTs);
      dailyAcc[slot].icon = icon;
      dailyAcc[slot].bestHourDistance = 99;
    }

    if (tempMin < dailyAcc[slot].minTemp) dailyAcc[slot].minTemp = tempMin;
    if (tempMax > dailyAcc[slot].maxTemp) dailyAcc[slot].maxTemp = tempMax;
    if (rainPct > dailyAcc[slot].rainPct) dailyAcc[slot].rainPct = rainPct;

    int distToMidday = abs(tmv.tm_hour - 12);
    if (distToMidday < dailyAcc[slot].bestHourDistance) {
      dailyAcc[slot].bestHourDistance = distToMidday;
      dailyAcc[slot].icon = icon;
    }
  }

  int visibleDaily = 0;
  for (int i = 0; i < dailyFilled && visibleDaily < DAILY_COUNT; ++i) {
    if (!dailyAcc[i].used) continue;
    if (dailyAcc[i].minTemp == 1000 || dailyAcc[i].maxTemp == -1000) continue;
    out.daily[visibleDaily].dayLabel = dailyAcc[i].dayLabel;
    out.daily[visibleDaily].icon = dailyAcc[i].icon;
    out.daily[visibleDaily].minTemp = dailyAcc[i].minTemp;
    out.daily[visibleDaily].maxTemp = dailyAcc[i].maxTemp;
    out.daily[visibleDaily].rainPct = dailyAcc[i].rainPct;
    visibleDaily++;
  }

  out.dailyCount = visibleDaily;
  return visibleDaily > 0;
}

void weather_begin() {
  ensureWiFi();
  ensureTimeConfigured();
}

bool weather_fetch(WeatherData& out) {
  ensureWiFi();
  ensureTimeConfigured();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[weather] WiFi not connected");
    return false;
  }

  String common = String("lat=") + WEATHER_LAT +
                  "&lon=" + WEATHER_LON +
                  "&units=" + WEATHER_UNITS +
                  "&lang=" + WEATHER_LANG +
                  "&appid=" + OPENWEATHER_KEY;

  String currentUrl = String("https://api.openweathermap.org/data/2.5/weather?") + common;
  String forecastUrl = String("https://api.openweathermap.org/data/2.5/forecast?") + common;

  Serial.println("[weather] Requesting current:");
  Serial.println(currentUrl);
  int currentCode = 0;
  String currentPayload = fetchUrl(currentUrl, currentCode);
  Serial.print("[weather] Current HTTP code: ");
  Serial.println(currentCode);
  if (currentCode != HTTP_CODE_OK) {
    Serial.println("[weather] Current error payload:");
    Serial.println(currentPayload);
    return false;
  }

  long tzOffsetSeconds = 0;
  time_t currentLocalTs = 0;
  if (!parseCurrentPayload(currentPayload, out, tzOffsetSeconds, currentLocalTs)) {
    return false;
  }

  Serial.println("[weather] Requesting forecast:");
  Serial.println(forecastUrl);
  int forecastCode = 0;
  String forecastPayload = fetchUrl(forecastUrl, forecastCode);
  Serial.print("[weather] Forecast HTTP code: ");
  Serial.println(forecastCode);
  if (forecastCode != HTTP_CODE_OK) {
    Serial.println("[weather] Forecast error payload:");
    Serial.println(forecastPayload);
    return false;
  }

  if (!parseForecastPayload(forecastPayload, out, tzOffsetSeconds)) {
    return false;
  }

  out.lastUpdated = fmtStamp(currentLocalTs);
  Serial.println("[weather] Weather updated OK (free plan endpoints)");
  return true;
}
