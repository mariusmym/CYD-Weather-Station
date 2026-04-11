#pragma once

#include <Arduino.h>

struct TouchEvent {
  int16_t x = 0;
  int16_t y = 0;
};

void touch_begin();
bool touch_read(TouchEvent& ev);
