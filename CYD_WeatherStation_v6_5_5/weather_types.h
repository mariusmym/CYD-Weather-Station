#pragma once

#include <Arduino.h>
#include "config.h"

struct CurrentWeather {
  String description;
  String icon;
  int temp = 0;
  int feelsLike = 0;
  int humidity = 0;
  int windKph = 0;
  int pressure = 0;
  int uv = 0;
  int cloudPct = 0;
  int visibilityKm = 0;
  uint32_t sunrise = 0;
  uint32_t sunset = 0;
};

struct HourlyWeather {
  String timeLabel;
  String icon;
  int temp = 0;
  int rainPct = 0;
};

struct DailyWeather {
  String dayLabel;
  String icon;
  int minTemp = 0;
  int maxTemp = 0;
  int rainPct = 0;
};

struct WeatherData {
  CurrentWeather current;
  HourlyWeather hourly[HOURLY_COUNT];
  DailyWeather daily[DAILY_COUNT];
  uint8_t hourlyCount = 0;
  uint8_t dailyCount = 0;
  String lastUpdated;
  long tzOffsetSeconds = 0;
};
