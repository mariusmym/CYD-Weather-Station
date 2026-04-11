#pragma once

#include "weather_types.h"

void weather_begin();
bool weather_fetch(WeatherData& out);
