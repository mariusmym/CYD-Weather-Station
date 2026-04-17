# CYD-Weather-Station
ESP32 CYD weather station with OpenWeather free API, dark/light themes, SD logging, touch UI, and auto-backlight timeout

A weather station firmware for the  **CYD2USB ESP32-2432S028R** (Cheap Yellow Display with 2 USB ports) using the free OpenWeather API.

Hardware reference: [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)

<img width="408" height="235" alt="CYD2USB-ST7789" src="https://github.com/user-attachments/assets/0118b8dc-7d46-406f-a9bf-24ec9ea4fef6" />

The image below is **just a mockup** for the first two tabs, based on the dark theme colors and layout constants from the code. The actual TFT rendering will look slightly different due to the bitmap fonts and pixel-level drawing, but the layout, colors and proportions should be close. 
More *real life* pictures in the images folder.

<img width="1199" height="448" alt="mockUp-" src="https://github.com/user-attachments/assets/f6693776-b4dd-4324-b93d-9b0750e4b53a" />


## Features

- **Current weather** - temperature, feels-like, humidity, wind, pressure, sunrise/sunset
- **3-hour forecast** - next 4 forecast slots with icons, temperature and rain probability
- **5-day daily view** - aggregated min/max temperatures and rain chance per day
- **Dark / Light theme** - toggle on the Status tab, persists across reboots (stored in ESP32 NVS flash)
- **SD card data logger** - logs weather readings to CSV on a micro-SD card (**SD card has be formated as FAT32**)
  - Manual logging via "log to SD" button on the Status tab
  - Automatic daily log at 12:00 local time
  - SD card presence indicator in the header bar (only updates at boot or when you log data onto the SD card)
- **Live header** - location pin, city name, clock (synced via NTP), Wi-Fi signal bars, SD card icon
- **Touch navigation** - four tabs (Now, Hourly, Daily, Status) plus a refresh button
- **Status tab** - state, last update, local IP, SD card status, uptime, restart/log/theme buttons
- **Backlight timeout** - screen turns off after inactivity, wakes instantly on touch
- **Restart button** - soft restart from the Status tab

## Hardware

| Component | Notes |
|-----------|-------|
| Board | ESP32-2432S028R (CYD) |
| Display | 2.8" ILI9341 320x240 TFT via VSPI |
| Touch | XPT2046 resistive, dedicated SPI pins (GPIO 25/39/32/33) |
| SD Card | Micro-SD slot, uses HSPI bus (GPIO 18/19/23, CS on GPIO 5) |
| Backlight | GPIO 21 |

## Pin Map

```
TFT (VSPI)      - managed by TFT_eSPI library (User_Setup.h)
Touch           - SCLK:25  MISO:39  MOSI:32  CS:33  IRQ:36
SD Card (HSPI)  - SCLK:18  MISO:19  MOSI:23  CS:5
Backlight       - GPIO 21
```

## Dependencies

Install these libraries in Arduino IDE (Library Manager or manual):

- **TFT_eSPI** by Bodmer - display driver
- **XPT2046_Touchscreen** by Paul Stoffregen - touch input
- **ArduinoJson** by Benoit Blanchon - API response parsing
- **SD** (built-in) - micro-SD card access

## Setup

1. Configure `TFT_eSPI`: keep your working `User_Setup.h` in the TFT_eSPI library folder.
2. Copy `secrets.example.h` to `secrets.h` and fill in your credentials:
   ```cpp
   #define WIFI_SSID       "YourSSID"
   #define WIFI_PASSWORD   "YourPassword"
   #define OPENWEATHER_KEY "your_api_key_here"
   #define WEATHER_CITY_NAME "Your City, CountryCode (2 or 3 letters)"
   #define WEATHER_LAT "00.0000"
   #define WEATHER_LON "00.0000"
   #define WEATHER_UNITS "metric"   // or "imperial"
   #define WEATHER_LANG  "en"
   ```
3. Get a free OpenWeather API key at [openweathermap.org](https://openweathermap.org/api). This firmware uses the free `/data/2.5/weather` and `/data/2.5/forecast` endpoints (no paid One Call API needed).
4. Compile and upload. The Arduino IDE project folder and `.ino` file must share the same name.

## SD Card Logging

Weather data is logged as CSV to `/log/YYYY-MM.csv` on the SD card. Each entry records:

```
datetime, temp, feels, humidity, wind_kph, pressure, clouds, description
```

The SD card uses a dedicated HSPI bus to avoid conflicts with the display (VSPI) and touch controller. If your CYD variant has different SD pins, adjust `sd_logger.cpp`.

## Backlight Timeout

The display backlight turns off after 3 minutes of no touch input to reduce power consumption and prevent LCD image persistence. Any touch immediately wakes the screen. The timeout is configurable via `BACKLIGHT_TIMEOUT_MS` in `config.h`.

## Touch Calibration

If touches are offset, adjust these values in `config.h`:

```
TOUCH_RAW_X_MIN / TOUCH_RAW_X_MAX
TOUCH_RAW_Y_MIN / TOUCH_RAW_Y_MAX
TOUCH_SWAP_XY / TOUCH_INVERT_X / TOUCH_INVERT_Y
```

Set `TOUCH_DEBUG = true` temporarily and watch Serial output while tapping corners to find correct raw ranges.

## File Structure

```
CYD_Weather_v6_5_5/
├── CYD_Weather_v6_5_5.ino   Main sketch
├── config.h                  Themes, layout, pins, timing
├── secrets.h                 WiFi + API credentials (git-ignored)
├── theme.cpp                 Runtime theme switching + NVS persistence
├── display_manager.h/.cpp    TFT init
├── touch_input.h/.cpp        Touch controller driver
├── weather_client.h/.cpp     OpenWeather API client + JSON parsing
├── weather_types.h           Data structures
├── ui.h/.cpp                 All UI drawing + touch handling
├── icons.h/.cpp              Procedural weather icons
├── sd_logger.h/.cpp          SD card CSV logger (HSPI)
└── README.md
```

## Notes

The **dark theme** is the primary and most polished experience. The light theme is functional but still a work in progress — colors and contrast may be refined in future updates.

The photos in this repository were taken with a phone camera and don't accurately represent the on-screen colors. TFT displays are notoriously difficult to photograph — the actual result looks significantly better in person.

## Version History

- **v6.5.5** - Location pin icon in header, improved SD card icon with contact pins, local IP on Status tab, backlight auto-off after 3 min idle, cleaned up debug output
- **v6.5.3** - Dedicated HSPI bus for SD card, restart button, temporary status messages, touch recovery after SD access
- **v6.4.x** - Runtime dark/light theme toggle, SD card logger, refined color palettes
- **v6.3.2** - Original TFT_eSPI rewrite with free OpenWeather plan support

## License

Personal / educational use. The OpenWeather API key is subject to their terms of service.


