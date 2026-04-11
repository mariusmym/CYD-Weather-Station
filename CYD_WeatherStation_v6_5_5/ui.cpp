#include "ui.h"
#include "display_manager.h"
#include "config.h"
#include "icons.h"
#include "secrets.h"
#include <WiFi.h>
#include <time.h>

static uint8_t sActiveTab = 0;
static String sStatus = "Starting";
static WeatherData sData;
static bool sHasData = false;
static bool sFetching = false;
static unsigned long sLastHeaderTick = 0;
static int sLastMinuteDrawn = -1;
static int sLastWifiLevel = -99;
static bool sLastWifiConnected = false;
static bool sSDCardPresent = false;


namespace {
  constexpr int kRefreshBtnW = 74;
  constexpr int kRefreshBtnH = 22;
  constexpr int kRefreshBtnMarginRight = 8;
  constexpr int kRefreshBtnMarginTop = 8;
  constexpr int kRefreshUpdatedGap = 6;

  constexpr int kFooterTabW = 72;
  constexpr int kFooterTabH = 26;
  constexpr int kFooterTabY = SCREEN_H - 32;
  constexpr int kFooterTabX0 = 4;
  constexpr int kFooterTabStep = 79;

  constexpr int kHourlyCardCount = 4;
  constexpr int kHourlyCardGap = 6;
  constexpr int kHourlyCardY = CONTENT_Y + 10;
  constexpr int kHourlyCardH = 152;
  constexpr int kHourlyCardW = (SCREEN_W - (kHourlyCardGap * (kHourlyCardCount + 1))) / kHourlyCardCount;

  constexpr int kDailyMaxVisibleRows = 5;
  constexpr int kDailyRowX = 8;
  constexpr int kDailyRowW = 304;
  constexpr int kDailyRowH = 26;
  constexpr int kDailyRowStep = 32;
  constexpr int kDailyRowStartY = CONTENT_Y + 8;
  constexpr int kDailyIconX = 80;
  constexpr int kDailyIconYInset = 4;
  constexpr int kDailyIconSize = 16;

  constexpr int kStatusCardX = 12;
  constexpr int kStatusCardY = CONTENT_Y + 14;
  constexpr int kStatusCardW = 296;
  constexpr int kStatusCardH = 120;
  constexpr int kStatusLeftX = 24;
  constexpr int kStatusRightX = 296;
  constexpr int kStatusTitleY = CONTENT_Y + 20;
  constexpr int kStatusValueY = CONTENT_Y + 52;
  constexpr int kStatusRow1Y = CONTENT_Y + 74;
  constexpr int kStatusRowStep = 18;

  constexpr int kRestartBtnW = 70;
  constexpr int kRestartBtnH = 20;
  constexpr int kRestartBtnX = 12;
  constexpr int kRestartBtnY = CONTENT_Y + 140;

  constexpr int kSDLogBtnW = 96;
  constexpr int kSDLogBtnH = 20;
  constexpr int kSDLogBtnX = 88;
  constexpr int kSDLogBtnY = CONTENT_Y + 140;

  constexpr int kThemeBtnW = 121;
  constexpr int kThemeBtnH = 20;
  constexpr int kThemeBtnX = 186;
  constexpr int kThemeBtnY = CONTENT_Y + 140;
}

static String fmtHM(time_t ts) {
  struct tm tmv;
  localtime_r(&ts, &tmv);
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &tmv);
  return String(buf);
}

static String getShortTime(const String& t) {
  if (t.length() >= 5) return t.substring(t.length() - 5);
  return t;
}

static String fmtClockNow() {
  if (!sHasData) return String("--:--");
  time_t utcNow = time(nullptr);
  if (utcNow < 100000) return String("--:--");
  time_t localTs = utcNow + sData.tzOffsetSeconds;
  struct tm tmv;
  gmtime_r(&localTs, &tmv);
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &tmv);
  return String(buf);
}

static int currentMinuteValue() {
  if (!sHasData) return -1;
  time_t utcNow = time(nullptr);
  if (utcNow < 100000) return -1;
  time_t localTs = utcNow + sData.tzOffsetSeconds;
  struct tm tmv;
  gmtime_r(&localTs, &tmv);
  return tmv.tm_hour * 60 + tmv.tm_min;
}

static int wifiSignalLevel() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  long rssi = WiFi.RSSI();
  if (rssi >= -55) return 4;
  if (rssi >= -67) return 3;
  if (rssi >= -75) return 2;
  if (rssi >= -85) return 1;
  return 0;
}

static uint16_t rainColor(int pct) {
  if (pct == 0) return UI_ORANGE;   // sunny / dry
  if (pct < 25) return UI_GREEN;    // very low
  if (pct < 50) return UI_OLIVE;    // moderate
  if (pct < 80) return UI_BLUE;     // likely rain
  return UI_BLUE;                   // heavy rain (same color)
}

static int refreshButtonX() { return SCREEN_W - kRefreshBtnW - kRefreshBtnMarginRight; }
static int refreshButtonY() { return CONTENT_Y + kRefreshBtnMarginTop; }

static void drawWifiIcon(int x, int y, int level, bool connected) {
  const int barW = 3, gap = 2, baseY = y + 13;
  const int heights[4] = {4, 7, 10, 13};
  tft.fillRect(x, y, 22, 14, UI_BG_2);
  if (!connected) {
    tft.drawLine(x + 3, y + 3, x + 18, y + 12, UI_RED);
    tft.drawLine(x + 18, y + 3, x + 3, y + 12, UI_RED);
    return;
  }
  for (int i = 0; i < 4; ++i) {
    int bx = x + i * (barW + gap);
    uint16_t col = ((i + 1) <= level) ? UI_GREEN : UI_BG_1;
    tft.fillRoundRect(bx, baseY - heights[i], barW, heights[i], 1, col);
  }
}

// Micro-SD card icon (14×14 area). Drawn at (x, y).
// Shape: rectangle with chamfered top-left corner.
static void drawSdIndicator(int x, int y, bool present) {
  const int W = 10, H = 13;            // card body
  const int ox = x + 2, oy = y;        // offset to center in 14px area
  const int notch = 3;                  // top-left chamfer size

  uint16_t body = present ? UI_GREEN : UI_TEXT_SOFT;

  // Clear area
  tft.fillRect(x, y, 14, 14, UI_BG_2);

  // Card body (main rectangle minus chamfer)
  tft.fillRect(ox, oy + notch, W, H - notch, body);           // lower portion
  tft.fillRect(ox + notch, oy, W - notch, notch, body);       // upper-right portion
  tft.fillTriangle(ox, oy + notch, ox + notch, oy, ox + notch, oy + notch, body); // chamfer fill

  // Status overlay
  if (!present) {
    // Red X over the card
    tft.drawLine(ox + 1, oy + 7, ox + W - 2, oy + H - 2, UI_TEXT_MAIN);
    tft.drawLine(ox + W - 2, oy + 7, ox + 1, oy + H - 2, UI_TEXT_MAIN);
  }

  // Contact pins at the top
  uint16_t pinCol = present ? UI_BG_2 : UI_BG_1;
  for (int i = 0; i < 4; i++) {
    int px = ox + 1 + i * 2;
    tft.drawFastVLine(px + 1, oy + 2, 3, pinCol);
  }
}

static void drawTempWithUnitLeft(int x, int y, int temp, int font, uint16_t fg, uint16_t bg) {
  String value = String(temp);
  tft.setTextColor(fg, bg);
  tft.drawString(value, x, y, font);
  int valueW = tft.textWidth(value, font);
  int circleX = x + valueW + ((font >= 6) ? 8 : 5);
  int circleY = y + ((font >= 6) ? 8 : 5);
  int circleR = (font >= 6) ? 3 : 2;
  tft.drawCircle(circleX, circleY, circleR, fg);
  tft.drawString("C", circleX + circleR + 4, y, font >= 6 ? 4 : 2);
}

static void drawTempWithUnitCenter(int cx, int y, int temp, int font, uint16_t fg, uint16_t bg) {
  String value = String(temp);
  int totalW = tft.textWidth(value, font) + 14 + tft.textWidth("C", 2);
  int x = cx - totalW / 2;
  tft.setTextColor(fg, bg);
  tft.drawString(value, x, y, font);
  int valueW = tft.textWidth(value, font);
  int circleX = x + valueW + 5;
  int circleY = y + ((font >= 4) ? 6 : 5);
  tft.drawCircle(circleX, circleY, 2, fg);
  tft.drawString("C", circleX + 5, y, 2);
}

static void drawHeader() {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, UI_BG_2);
  tft.drawFastHLine(0, HEADER_H - 1, SCREEN_W, UI_BG_0);

  // Location pin (drawn before city name)
  int px = 5, py = 7;
  tft.fillCircle(px + 4, py + 4, 3, UI_RED);
  tft.fillTriangle(px + 1, py + 5, px + 7, py + 5, px + 4, py + 12, UI_RED);
  tft.fillCircle(px + 4, py + 4, 2, UI_BG_2);  // hollow center

  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawString(WEATHER_CITY_NAME, 18, 6, 2);

  if (sFetching) {
    int dotX = 18 + tft.textWidth(WEATHER_CITY_NAME, 2) + 8;
    tft.fillCircle(dotX, 11, 3, UI_GREEN);
  }

  // SD indicator + Wi-Fi icon
  bool connected = (WiFi.status() == WL_CONNECTED);
  int wifiLevel = wifiSignalLevel();
  drawSdIndicator(SCREEN_W - 88, 6, sSDCardPresent);
  drawWifiIcon(SCREEN_W - 72, 6, wifiLevel, connected);

  tft.setTextColor(connected ? UI_TEXT_MAIN : UI_TEXT_SOFT, UI_BG_2);
  tft.drawRightString(fmtClockNow(), SCREEN_W - 8, 6, 2);

  sLastWifiConnected = connected;
  sLastWifiLevel = wifiLevel;
  sLastMinuteDrawn = currentMinuteValue();
}

static void drawFooter() {
  tft.fillRect(0, SCREEN_H - FOOTER_H, SCREEN_W, FOOTER_H, UI_BG_2);
  tft.drawFastHLine(0, SCREEN_H - FOOTER_H, SCREEN_W, UI_BG_0);

  const char* tabs[4] = {"now", "hourly", "daily", "status"};

  for (int i = 0; i < 4; ++i) {
    int x = kFooterTabX0 + i * kFooterTabStep;
    bool active = (i == sActiveTab);

    uint16_t fill = active ? UI_BG_0 : UI_BG_1;
    uint16_t text = active ? UI_TEXT_MAIN : UI_TEXT_SOFT;

    tft.fillRoundRect(x, kFooterTabY, kFooterTabW, kFooterTabH, 6, fill);

    if (active) {
      tft.drawRoundRect(x, kFooterTabY, kFooterTabW, kFooterTabH, 6, UI_GREEN);
      tft.fillRect(x + 10, SCREEN_H - 8, kFooterTabW - 20, 3, UI_GREEN);
    } else {
      tft.drawRoundRect(x, kFooterTabY, kFooterTabW, kFooterTabH, 6, UI_BG_0);
    }

    tft.setTextColor(text, fill);
    tft.drawCentreString(tabs[i], x + kFooterTabW / 2, kFooterTabY + 6, 2);
  }
}

static void clearContent() {
  tft.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, UI_BG_1);
}

static void drawRefreshButton() {
  const int x = refreshButtonX();
  const int y = refreshButtonY();
  tft.fillRoundRect(x, y, kRefreshBtnW, kRefreshBtnH, 5, UI_BG_2);
  tft.drawRoundRect(x, y, kRefreshBtnW, kRefreshBtnH, 5, UI_GREEN);
  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawCentreString("refresh", x + kRefreshBtnW / 2, y + 4, 2);
}

static void drawNow() {
  clearContent();
  drawRefreshButton();

  if (!sHasData) {
    tft.setTextColor(UI_TEXT_SOFT, UI_BG_1);
    tft.drawString("Waiting for weather data...", 14, CONTENT_Y + 44, 2);
    return;
  }

  icons_drawWeather(18, 46, 72, sData.current.icon);

  drawTempWithUnitLeft(106, 52, sData.current.temp, 6, UI_TEXT_MAIN, UI_BG_1);
  tft.setTextColor(UI_TEXT_SOFT, UI_BG_1);
  tft.drawString(sData.current.description, 108, 102, 2);
  tft.drawCentreString(
    "updated " + getShortTime(sData.lastUpdated),
    refreshButtonX() + kRefreshBtnW / 2,
    refreshButtonY() + kRefreshBtnH + kRefreshUpdatedGap,
    2
  );

  tft.fillRoundRect(14, 122, 292, 66, 8, UI_BG_2);
  tft.drawFastVLine(18, 130, 48, UI_GREEN);

  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawString("feels", 24, 128, 2);
  tft.drawString("humidity", 24, 146, 2);
  tft.drawString("sunrise", 24, 164, 2);

  tft.drawString("wind", 164, 128, 2);
  tft.drawString("pressure", 164, 146, 2);
  tft.drawString("sunset", 164, 164, 2);

  drawTempWithUnitLeft(110, 128, sData.current.feelsLike, 2, UI_GREEN, UI_BG_2);
  tft.setTextColor(UI_GREEN, UI_BG_2);
  tft.drawRightString(String(sData.current.humidity) + "%", 138, 146, 2);
  tft.drawRightString(fmtHM(sData.current.sunrise), 138, 164, 2);

  tft.drawRightString(String(sData.current.windKph) + "km/h", 292, 128, 2);
  tft.drawRightString(String(sData.current.pressure) + "hPa", 292, 146, 2);
  tft.drawRightString(fmtHM(sData.current.sunset), 292, 164, 2);
}

static void drawHourly() {
  clearContent();
  if (!sHasData) {
    tft.setTextColor(UI_TEXT_SOFT, UI_BG_1);
    tft.drawString("No forecast yet", 14, CONTENT_Y + 18, 2);
    return;
  }

  int count = sData.hourlyCount > 0 ? sData.hourlyCount : HOURLY_COUNT;
  if (count > kHourlyCardCount) count = kHourlyCardCount;

  for (int i = 0; i < count; ++i) {
    int x = kHourlyCardGap + i * (kHourlyCardW + kHourlyCardGap);
    int y = kHourlyCardY;

    tft.fillRoundRect(x, y, kHourlyCardW, kHourlyCardH, 8, UI_BG_2);
    tft.fillRect(x + 8, y + 30, kHourlyCardW - 16, 2, UI_OLIVE);

    tft.setTextColor(UI_OLIVE, UI_BG_2);
    tft.drawCentreString(sData.hourly[i].timeLabel, x + kHourlyCardW / 2, y + 10, 2);
    icons_drawWeather(x + (kHourlyCardW - 44) / 2, y + 30, 44, sData.hourly[i].icon);
    drawTempWithUnitCenter(x + kHourlyCardW / 2, y + 92, sData.hourly[i].temp, 4, UI_TEXT_MAIN, UI_BG_2);
    tft.setTextColor(UI_BLUE, UI_BG_2);
    tft.drawCentreString(String(sData.hourly[i].rainPct) + "% rain", x + kHourlyCardW / 2, y + 124, 2);
  }
}

static void drawDaily() {
  clearContent();
  if (!sHasData) {
    tft.setTextColor(UI_TEXT_SOFT, UI_BG_1);
    tft.drawString("No daily data yet", 14, CONTENT_Y + 18, 2);
    return;
  }

  int count = sData.dailyCount;
  if (count > kDailyMaxVisibleRows) count = kDailyMaxVisibleRows;

  for (int i = 0; i < count; ++i) {
    if (sData.daily[i].dayLabel.length() == 0) continue;

    int y = kDailyRowStartY + i * kDailyRowStep;
    uint16_t fill = UI_BG_2;

    tft.fillRoundRect(kDailyRowX, y, kDailyRowW, kDailyRowH, 6, fill);
    tft.drawFastHLine(kDailyRowX + 8, y + kDailyRowH - 2, kDailyRowW - 16, UI_BG_0);

    tft.setTextColor(UI_OLIVE, fill);
    tft.drawString(sData.daily[i].dayLabel, 16, y + 6, 2);
    icons_drawWeather(kDailyIconX, y + kDailyIconYInset, kDailyIconSize, sData.daily[i].icon);

    tft.setTextColor(UI_TEXT_MAIN, fill);
    tft.drawString(String(sData.daily[i].minTemp) + "/" + String(sData.daily[i].maxTemp) + " C", 118, y + 6, 2);

    tft.setTextColor(rainColor(sData.daily[i].rainPct), fill);
    tft.drawRightString(String(sData.daily[i].rainPct) + "%", 296, y + 6, 2);
  }
}

// Small rounded button helper
static void drawSmallButton(int x, int y, int w, int h, const char* label, uint16_t border) {
  tft.fillRoundRect(x, y, w, h, 4, UI_BG_2);
  tft.drawRoundRect(x, y, w, h, 4, border);
  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawCentreString(label, x + w / 2, y + 3, 2);
}

static void drawStatus() {
  clearContent();

  tft.fillRoundRect(kStatusCardX, kStatusCardY, kStatusCardW, kStatusCardH, 8, UI_BG_2);

  // Title row
  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawString("status / settings", kStatusLeftX, kStatusTitleY, 2);
  tft.setTextColor(UI_GREEN, UI_BG_2);
  tft.drawRightString("v" FW_VERSION, kStatusRightX, kStatusTitleY, 2);

  tft.drawFastHLine(kStatusCardX + 10, kStatusCardY + 26, kStatusCardW - 20, UI_BG_0);

  // Status text
  uint16_t statusColor = UI_GREEN;
  if (sStatus.indexOf("fail") >= 0 || sStatus.indexOf("No data") >= 0 || sStatus.indexOf("Error") >= 0) {
    statusColor = UI_RED;
  }

  String statusText = sStatus;
  const int maxLen = 18;
  if (statusText.length() > maxLen) {
    statusText = statusText.substring(0, maxLen - 3) + "...";
  }

  // State row: label + value on same line
  tft.setTextColor(UI_TEXT_SOFT, UI_BG_2);
  tft.drawString("state", kStatusLeftX, CONTENT_Y + 42, 2);
  tft.setTextColor(statusColor, UI_BG_2);
  tft.drawRightString(statusText, kStatusRightX, CONTENT_Y + 42, 2);

  // System info rows raised up
  tft.setTextColor(UI_TEXT_SOFT, UI_BG_2);
  tft.drawString("updated", kStatusLeftX, CONTENT_Y + 60, 2);
  tft.drawString("local ip", kStatusLeftX, CONTENT_Y + 78, 2);
  tft.drawString("sd card", kStatusLeftX, CONTENT_Y + 96, 2);

  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawRightString(sHasData ? sData.lastUpdated : "--", kStatusRightX, CONTENT_Y + 60, 2);

  if (WiFi.status() == WL_CONNECTED) {
    tft.drawRightString(WiFi.localIP().toString(), kStatusRightX, CONTENT_Y + 78, 2);
  } else {
    tft.drawRightString("not connected", kStatusRightX, CONTENT_Y + 78, 2);
  }

  tft.drawRightString(sSDCardPresent ? "present" : "not inserted", kStatusRightX, CONTENT_Y + 96, 2);

  // UpTime logic
  unsigned long sec = millis() / 1000;
  int d = sec / 86400; int h = (sec % 86400) / 3600; int m = (sec % 3600) / 60;
  char uptimeBuf[24];
  snprintf(uptimeBuf, sizeof(uptimeBuf), "%dd %dh %dm", d, h, m);
  tft.setTextColor(UI_TEXT_SOFT, UI_BG_2);
  tft.drawString("uptime", kStatusLeftX, CONTENT_Y + 112, 2);
  tft.setTextColor(UI_TEXT_MAIN, UI_BG_2);
  tft.drawRightString(uptimeBuf, kStatusRightX, CONTENT_Y + 112, 2);

  drawSmallButton(kRestartBtnX, kRestartBtnY, kRestartBtnW, kRestartBtnH, "restart", UI_RED);
  drawSmallButton(kSDLogBtnX, kSDLogBtnY, kSDLogBtnW, kSDLogBtnH, "log to SD", UI_GREEN);

  const char* themeLabel = themeDark ? "light mode" : "dark mode";
  drawSmallButton(kThemeBtnX, kThemeBtnY, kThemeBtnW, kThemeBtnH, themeLabel, UI_OLIVE);
}

static void redrawAll() {
  drawHeader();
  drawFooter();
  switch (sActiveTab) {
    case 0: drawNow(); break;
    case 1: drawHourly(); break;
    case 2: drawDaily(); break;
    default: drawStatus(); break;
  }
}

void ui_begin() {
  redrawAll();
}

void ui_setStatus(const String& text) {
  sStatus = text;
  if (sActiveTab == 3) drawStatus();
}

void ui_setFetching(bool fetching) {
  sFetching = fetching;
  drawHeader();
}

void ui_applyWeather(const WeatherData& data) {
  sData = data;
  sHasData = true;
  sFetching = false;
  redrawAll();
}

void ui_tick(const WeatherData* data) {
  (void)data;
  if (millis() - sLastHeaderTick < UI_HEADER_REFRESH_MS) return;
  sLastHeaderTick = millis();

  int minuteNow = currentMinuteValue();
  bool connected = (WiFi.status() == WL_CONNECTED);

  // Only redraw header when clock minute changes or connection state flips.
  // Ignore wifi signal level changes to avoid RSSI-bounce redraws.
  if (minuteNow != sLastMinuteDrawn || connected != sLastWifiConnected) {
    drawHeader();
  }
}

UIAction ui_handleTouch(int16_t x, int16_t y) {
  // Status tab action buttons first, so the footer does not steal those touches.
  if (sActiveTab == 3) {
    if (x >= kRestartBtnX && x < (kRestartBtnX + kRestartBtnW) &&
      y >= kRestartBtnY && y < (kRestartBtnY + kRestartBtnH)) {
    return UI_ACTION_RESTART;
    }
    if (x >= kThemeBtnX && x < (kThemeBtnX + kThemeBtnW) &&
        y >= kThemeBtnY && y < (kThemeBtnY + kThemeBtnH)) {
      return UI_ACTION_TOGGLE_THEME;
    }

    if (x >= kSDLogBtnX && x < (kSDLogBtnX + kSDLogBtnW) &&
        y >= kSDLogBtnY && y < (kSDLogBtnY + kSDLogBtnH)) {
      return UI_ACTION_SD_LOG;
    }
  }

  // Refresh button on the Now tab.
  if (sActiveTab == 0) {
    const int bx = refreshButtonX();
    const int by = refreshButtonY();
    if (x >= bx && x < bx + kRefreshBtnW &&
        y >= by && y < by + kRefreshBtnH) {
      return UI_ACTION_REFRESH;
    }
  }

  // Footer tabs last.
  if (y >= SCREEN_H - FOOTER_H - 2) {
    uint8_t newTab = sActiveTab;

    if (x >= 0 && x < 79) newTab = 0;
    else if (x >= 79 && x < 158) newTab = 1;
    else if (x >= 158 && x < 237) newTab = 2;
    else if (x >= 237 && x <= 319) newTab = 3;

    if (newTab != sActiveTab) {
      sActiveTab = newTab;
      redrawAll();
      return (UIAction)(sActiveTab + 1);
    }

    return UI_ACTION_NONE;
  }

  return UI_ACTION_NONE;
}

void ui_setSDCardPresent(bool present) {
  sSDCardPresent = present;
  drawHeader();
}

//color test 
/*
void ui_colorTest() {
  Serial.println("[color test] starting...");
  tft.fillScreen(TFT_RED);   delay(1000);
  tft.fillScreen(TFT_GREEN); delay(1000);
  tft.fillScreen(TFT_BLUE);  delay(1000);
  tft.fillScreen(UI_ORANGE);  delay(1500);
  tft.fillScreen(UI_BG_0);
}
*/