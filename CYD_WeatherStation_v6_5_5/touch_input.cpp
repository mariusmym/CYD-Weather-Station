#include "touch_input.h"
#include "config.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

static SPIClass touchSPI(VSPI);
static XPT2046_Touchscreen ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
static bool sPressed = false;
static uint32_t sPressedStartMs = 0;

static int16_t mapClamped(int32_t v, int32_t inMin, int32_t inMax, int32_t outMin, int32_t outMax) {
  if (v < inMin) v = inMin;
  if (v > inMax) v = inMax;
  return (int16_t)((v - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
}

void touch_begin() {
  touchSPI.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, TOUCH_CS_PIN);
  ts.begin(touchSPI);
  ts.setRotation(1);
}

bool touch_read(TouchEvent& ev) {
  bool touched = ts.touched();
  if (!touched) {
    sPressed = false;
    return false;
  }

  // fire once per press, after a short stable contact
  if (!sPressed) {
    sPressed = true;
    sPressedStartMs = millis();
    return false;
  }

  if (millis() - sPressedStartMs < 35) {
    return false;
  }

  TS_Point p = ts.getPoint();
  int32_t rawx0 = p.x;
  int32_t rawy0 = p.y;
  int32_t rx = rawx0;
  int32_t ry = rawy0;

  if (TOUCH_SWAP_XY) {
    int32_t tmp = rx;
    rx = ry;
    ry = tmp;
  }

  int16_t sx = mapClamped(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, SCREEN_W - 1);
  int16_t sy = mapClamped(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, SCREEN_H - 1);

  if (TOUCH_INVERT_X) sx = SCREEN_W - 1 - sx;
  if (TOUCH_INVERT_Y) sy = SCREEN_H - 1 - sy;

  ev.x = sx;
  ev.y = sy;
#if defined(ARDUINO)
  if (TOUCH_DEBUG) {
    Serial.printf("[touch] raw0=(%ld,%ld) mapped_raw=(%ld,%ld) screen=(%d,%d) flags swap=%d invX=%d invY=%d\n",
                  (long)rawx0, (long)rawy0, (long)rx, (long)ry, sx, sy,
                  TOUCH_SWAP_XY ? 1 : 0, TOUCH_INVERT_X ? 1 : 0, TOUCH_INVERT_Y ? 1 : 0);
  }
#endif
  // keep sPressed true until release so we only emit once
  sPressedStartMs = millis() - 1000;
  return true;
}

