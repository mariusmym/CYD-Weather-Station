#include "icons.h"
#include "display_manager.h"
#include "config.h"
#include <math.h>

static int sc(int base, int size, int ref = 52) {
  int v = (base * size) / ref;
  return v < 1 ? 1 : v;
}

static void sun(int x, int y, int size) {
  int r = sc(10, size);
  int rayGap = sc(5, size);
  int rayLen = sc(11, size);
  tft.fillCircle(x, y, r, UI_ORANGE);
  for (int i = 0; i < 8; ++i) {
    float a = i * 0.785398f;
    int x1 = x + (int)((r + rayGap) * cosf(a));
    int y1 = y + (int)((r + rayGap) * sinf(a));
    int x2 = x + (int)((r + rayLen) * cosf(a));
    int y2 = y + (int)((r + rayLen) * sinf(a));
    tft.drawLine(x1, y1, x2, y2, UI_ORANGE);
  }
}

static void cloud(int x, int y, int size, uint16_t c1, uint16_t c2) {
  int r1 = sc(9, size);
  int r2 = sc(12, size);
  int r3 = sc(10, size);
  int dx1 = sc(10, size);
  int dx3 = sc(12, size);
  int dyTop = sc(4, size);
  int bodyW = sc(40, size);
  int bodyH = sc(14, size);
  int bodyX = x - sc(18, size);
  int bodyY = y + sc(2, size);
  int radius = sc(7, size);

  tft.fillCircle(x - dx1, y + sc(2, size), r1, c1);
  tft.fillCircle(x, y - dyTop, r2, c1);
  tft.fillCircle(x + dx3, y + sc(2, size), r3, c1);
  tft.fillRoundRect(bodyX, bodyY, bodyW, bodyH, radius, c2);
}

static void rain(int x, int y, int size) {
  cloud(x, y, size, UI_CLOUD, UI_CLOUD_DARK);
  int y1 = y + sc(20, size);
  int y2 = y + sc(28, size);
  tft.drawLine(x - sc(10, size), y1, x - sc(14, size), y2, UI_BLUE);
  tft.drawLine(x, y1, x - sc(4, size), y2, UI_BLUE);
  tft.drawLine(x + sc(10, size), y1, x + sc(6, size), y2, UI_BLUE);
}

static void storm(int x, int y, int size) {
  cloud(x, y, size, UI_CLOUD, UI_CLOUD_DARK);
  tft.fillTriangle(x + sc(2, size), y + sc(18, size), x - sc(4, size), y + sc(30, size), x + sc(3, size), y + sc(30, size), UI_ORANGE);
  tft.fillTriangle(x + sc(3, size), y + sc(30, size), x - sc(1, size), y + sc(40, size), x + sc(12, size), y + sc(25, size), UI_ORANGE);
  tft.fillTriangle(x + sc(12, size), y + sc(25, size), x + sc(4, size), y + sc(25, size), x + sc(2, size), y + sc(18, size), UI_ORANGE);
}

static void snow(int x, int y, int size) {
  cloud(x, y, size, UI_CLOUD, UI_CLOUD_DARK);
  for (int dx = -sc(10, size); dx <= sc(10, size); dx += sc(10, size)) {
    int cx = x + dx;
    int cy = y + sc(25, size);
    int r = sc(3, size);
    int r2 = sc(2, size);
    tft.drawLine(cx - r, cy, cx + r, cy, UI_TEXT_MAIN);
    tft.drawLine(cx, cy - r, cx, cy + r, UI_TEXT_MAIN);
    tft.drawLine(cx - r2, cy - r2, cx + r2, cy + r2, UI_TEXT_MAIN);
    tft.drawLine(cx - r2, cy + r2, cx + r2, cy - r2, UI_TEXT_MAIN);
  }
}

static void moon(int x, int y, int size, uint16_t bg) {
  int r = sc(10, size);
  tft.fillCircle(x, y, r, UI_TEXT_MAIN);
  tft.fillCircle(x + r / 3, y - r / 5, r, bg);
}

void icons_drawWeather(int16_t x, int16_t y, int16_t size, const String& iconCode) {
  String c = iconCode;
  bool night = c.endsWith("n");
  int cx = x + size / 2;
  int cy = y + size / 2;
  uint16_t bg = (size <= 24) ? UI_BG_2 : UI_BG_1;

  if (c.startsWith("01")) {
    if (night) moon(cx, cy, size, bg); else sun(cx, cy, size);
  } else if (c.startsWith("02")) {
    if (night) moon(cx - sc(8, size), cy - sc(5, size), size, bg); else sun(cx - sc(8, size), cy - sc(5, size), size);
    cloud(cx + sc(2, size), cy + sc(4, size), size, UI_CLOUD, UI_CLOUD_DARK);
  } else if (c.startsWith("03") || c.startsWith("04")) {
    cloud(cx, cy, size, UI_CLOUD, UI_CLOUD_DARK);
  } else if (c.startsWith("09") || c.startsWith("10")) {
    rain(cx, cy - sc(3, size), size);
  } else if (c.startsWith("11")) {
    storm(cx, cy - sc(3, size), size);
  } else if (c.startsWith("13")) {
    snow(cx, cy - sc(3, size), size);
  } else {
    tft.drawFastHLine(x + sc(6, size), cy - sc(8, size), size - sc(12, size), UI_CLOUD);
    tft.drawFastHLine(x + sc(2, size), cy, size - sc(4, size), UI_CLOUD_DARK);
    tft.drawFastHLine(x + sc(8, size), cy + sc(8, size), size - sc(16, size), UI_CLOUD);
  }
}
