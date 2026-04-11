#pragma once

#include "weather_types.h"

enum UIAction {
  UI_ACTION_NONE,
  UI_ACTION_TAB_NOW,
  UI_ACTION_TAB_HOURLY,
  UI_ACTION_TAB_DAILY,
  UI_ACTION_TAB_STATUS,
  UI_ACTION_REFRESH,
  UI_ACTION_TOGGLE_THEME,
  UI_ACTION_SD_LOG,
  UI_ACTION_RESTART
};

void ui_begin();
void ui_setStatus(const String& text);
void ui_setFetching(bool fetching);
void ui_setSDCardPresent(bool present);
void ui_applyWeather(const WeatherData& data);
void ui_tick(const WeatherData* data);
UIAction ui_handleTouch(int16_t x, int16_t y);
void ui_colorTest();
