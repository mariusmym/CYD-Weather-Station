#include "config.h"
#include "secrets.h"
#include "display_manager.h"
#include "touch_input.h"
#include "weather_client.h"
#include "ui.h"
#include "sd_logger.h"
#include <SPI.h>

static WeatherData gWeather;
static unsigned long gLastFetchMs = 0;
static bool gHaveWeather = false;
static bool gFetchInProgress = false;
static String gBaseStatus = "Booting...";
static unsigned long gTempStatusUntil = 0;
static unsigned long gLastTouchMs = 0;
static bool gScreenOn = true;

static void setBaseStatus(const String& text) {
  gBaseStatus = text;
  ui_setStatus(text);
}

static void showTempStatus(const String& text, uint32_t ms = 2500) {
  ui_setStatus(text);
  gTempStatusUntil = millis() + ms;
}

static void restoreBaseStatusIfNeeded() {
  if (gTempStatusUntil != 0 && (long)(millis() - gTempStatusUntil) >= 0) {
    gTempStatusUntil = 0;
    ui_setStatus(gBaseStatus);
  }
}

static void applyWeatherIfAvailable() {
  if (gHaveWeather) {
    ui_applyWeather(gWeather);
  }
}

static void recoverUIAfterSD() {
  // Release SD CS and re-arm touch after SD access.
  // Avoid full display/UI reinit here because that causes the white flash
  // and multiple redraws after each log attempt.
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(2);

  touch_begin();
  delay(2);
}

static bool fetchWeatherNow(bool manual) {
  gFetchInProgress = true;
  setBaseStatus("Connecting...");
  bool ok = weather_fetch(gWeather);
  gFetchInProgress = false;

  if (ok) {
    gHaveWeather = true;
    gLastFetchMs = millis();
    applyWeatherIfAvailable();
    setBaseStatus(manual ? "Updated" : "Online");
    return true;
  }

  setBaseStatus(gHaveWeather ? "Update failed" : "No data / check Serial");
  if (gHaveWeather) {
    applyWeatherIfAvailable();
  }
  return false;
}

static void screenWake() {
  if (!gScreenOn) {
    digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
    gScreenOn = true;
  }
  gLastTouchMs = millis();
}

static void screenSleep() {
  if (gScreenOn) {
    digitalWrite(TFT_BACKLIGHT_PIN, LOW);
    gScreenOn = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  theme_load();

  display_begin();
  gLastTouchMs = millis();
  touch_begin();

  sdlog_begin();

  ui_begin();
  setBaseStatus("Booting...");

  bool sdOk = sdlog_isCardPresent();
  ui_setSDCardPresent(sdOk);

  weather_begin();
  setBaseStatus("WiFi...");

  fetchWeatherNow(false);
}

void loop() {
  TouchEvent ev;
  if (touch_read(ev)) {
    screenWake();
    UIAction action = ui_handleTouch(ev.x, ev.y);

    if (action == UI_ACTION_REFRESH && !gFetchInProgress) {
      fetchWeatherNow(true);
    }
    else if (action == UI_ACTION_TOGGLE_THEME) {
      theme_set(!themeDark);
      display_begin();
      touch_begin();
      ui_begin();
      if (gHaveWeather) ui_applyWeather(gWeather);
      ui_setStatus(gBaseStatus);
    }
    else if (action == UI_ACTION_SD_LOG) {
      if (gHaveWeather) {
        showTempStatus("Logging...", 4000);
        delay(50);  // give TFT time to render before blocking SD check/write

        bool ok = sdlog_logNow(gWeather);
        ui_setSDCardPresent(ok);
        recoverUIAfterSD();
        showTempStatus(ok ? "SD log saved" : "SD write failed");
      } else {
        showTempStatus("No weather data yet");
      }
    }
    else if (action == UI_ACTION_RESTART) {
      showTempStatus("Restarting...", 2000);
      delay(150);
      ESP.restart();
    }
  }

  if (!gFetchInProgress && (millis() - gLastFetchMs >= WEATHER_REFRESH_MS)) {
    fetchWeatherNow(false);
  }

  if (gHaveWeather) {
    bool autoLogged = sdlog_tick(gWeather);
    if (autoLogged) {
      ui_setSDCardPresent(true);
      recoverUIAfterSD();
      showTempStatus("Auto log saved");
    }
  }

  restoreBaseStatusIfNeeded();
  ui_tick(gHaveWeather ? &gWeather : nullptr);
  if (gScreenOn && (millis() - gLastTouchMs >= BACKLIGHT_TIMEOUT_MS)) {
    screenSleep();
  }
  delay(15);
}
