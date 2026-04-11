#pragma once

#include <Arduino.h>
#include "weather_types.h"

void sdlog_begin();
bool sdlog_isCardPresent();
bool sdlog_logNow(const WeatherData& data);   // manual log
bool sdlog_tick(const WeatherData& data);      // auto-log at noon, returns true if logged
