#include "sd_logger.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>
#include <time.h>

static int sLastLogDay = -1;
static bool sSDReady = false;
static bool sSPIStarted = false;
static unsigned long sLastInitAttemptMs = 0;
static SPIClass sdSPI(HSPI);

static bool ensureSDReady() {
  if (sSDReady) return true;

  unsigned long nowMs = millis();
  if (sLastInitAttemptMs != 0 && (nowMs - sLastInitAttemptMs < 1000)) {
    return false;
  }
  sLastInitAttemptMs = nowMs;

  // Dedicated SPI bus for SD to avoid disturbing touch on VSPI.
  // Start HSPI only once to avoid repeated bus reinitialization on retries.
  if (!sSPIStarted) {
    sdSPI.begin(18, 19, 23, SD_CS_PIN);
    sSPIStarted = true;
  }

  if (SD.begin(SD_CS_PIN, sdSPI)) {
    Serial.println("[sd] Card OK");
    sSDReady = true;
    return true;
  }

  Serial.println("[sd] No card");
  sSDReady = false;
  return false;
}

void sdlog_begin() {
  Serial.println("[sd] Logger ready (lazy init, HSPI)");
}

bool sdlog_isCardPresent() {
  return ensureSDReady();
}

static String logFilePath(const WeatherData& data) {
  time_t utcNow = time(nullptr);
  time_t localTs = utcNow + data.tzOffsetSeconds;

  struct tm tmv;
  gmtime_r(&localTs, &tmv);

  char buf[24];
  snprintf(buf, sizeof(buf), "/log/%04d-%02d.csv", tmv.tm_year + 1900, tmv.tm_mon + 1);
  return String(buf);
}

bool sdlog_logNow(const WeatherData& data) {
  if (!ensureSDReady()) {
    return false;
  }

  if (!SD.exists("/log")) SD.mkdir("/log");

  String path = logFilePath(data);
  bool isNew = !SD.exists(path.c_str());

  File f = SD.open(path.c_str(), FILE_APPEND);
  if (!f) {
    Serial.println("[sd] Failed to open log file");
    sSDReady = false;
    return false;
  }

  if (isNew) {
    f.println("datetime,temp,feels,humidity,wind_kph,pressure,clouds,description");
  }

  time_t utcNow = time(nullptr);
  time_t localTs = utcNow + data.tzOffsetSeconds;

  struct tm tmv;
  gmtime_r(&localTs, &tmv);

  char ts[24];
  strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M", &tmv);

  char line[160];
  snprintf(line, sizeof(line), "%s,%d,%d,%d,%d,%d,%d,%s",
           ts,
           data.current.temp,
           data.current.feelsLike,
           data.current.humidity,
           data.current.windKph,
           data.current.pressure,
           data.current.cloudPct,
           data.current.description.c_str());
  f.println(line);
  f.close();

  Serial.printf("[sd] Logged to %s\n", path.c_str());
  sLastLogDay = tmv.tm_yday;
  return true;
}

bool sdlog_tick(const WeatherData& data) {
  time_t utcNow = time(nullptr);
  if (utcNow < 100000) return false;

  time_t localTs = utcNow + data.tzOffsetSeconds;
  struct tm tmv;
  gmtime_r(&localTs, &tmv);

  if (tmv.tm_hour == 12 && tmv.tm_min < 10 && tmv.tm_yday != sLastLogDay) {
    Serial.println("[sd] Auto log");
    return sdlog_logNow(data);
  }
  return false;
}
