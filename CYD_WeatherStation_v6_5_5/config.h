#pragma once

#include <Arduino.h>


// ============================================================
// Firmware version
// ============================================================

#define FW_VERSION "6.5.5"

// ============================================================
// Theme – runtime-switchable colour palettes
// ============================================================

struct ThemeColors {
  uint16_t bg0;
  uint16_t bg1;
  uint16_t bg2;
  uint16_t textMain;
  uint16_t textSoft;
  uint16_t green;
  uint16_t olive;
  uint16_t blue;
  uint16_t red;
  uint16_t orange;
  uint16_t cloud;
  uint16_t cloudDark;
};

// Refined dark palette – deep charcoal base, blue-grey accents
static constexpr ThemeColors THEME_DARK = {
  0x0841,   // #080808  near-black base
  0x18E3,   // #1E1E1E  card surfaces
  0x2965,   // #2A2D30  header / footer / buttons

  0xE73C,   // #E6E6E6  primary text – bright but not pure white
  0x8C71,   // #8C8E8C  secondary text

  0x3DEA,   // #3CBC50  muted green – less neon
  0x6DDF,   // #6DBBFF  cool blue-grey labels (replaces olive)
  0x5D5E,   // #5CAEF4  rain blue
  0xF26B,   // #F34D5A  warm red warnings
  0xFDA0,   // #FDB400  amber sun / warm accent

  0xae9d,   // #BDBDBD  light cloud
  0x4A69,   // #4B4D4B  dark cloud
};

// Refined light palette – warm paper, strong contrast
static constexpr ThemeColors THEME_LIGHT = {
  0xEF5D,   // bg0  (screen)      #EDE9DD
  0xFFFF,   // bg1  (content)     #FFFFFF  <-- cleaner base
  0xf7be,   // bg2  (cards)       #DDD8CC  <-- slightly darker than bg1

  0x2104,   // text main
  0x5AEB,   // text soft (was too bright -> darker grey)

  0x3566,   // green
  0xec01,   // olive/labels
  0x0312,   // blue
  0xC0C4,   // red
  0xEC80,   // orange

  0xae9d,   // cloud
  0x7BCF    // cloud dark
};
// Active theme – set at runtime
extern ThemeColors activeTheme;
extern bool themeDark;          // true = dark, false = light

void theme_set(bool dark);     // switch and persist
void theme_load();             // restore from NVS – call once in setup

// Convenience macros so existing code compiles with minimal edits
#define UI_BG_0       activeTheme.bg0
#define UI_BG_1       activeTheme.bg1
#define UI_BG_2       activeTheme.bg2
#define UI_TEXT_MAIN  activeTheme.textMain
#define UI_TEXT_SOFT  activeTheme.textSoft
#define UI_GREEN      activeTheme.green
#define UI_OLIVE      activeTheme.olive
#define UI_BLUE       activeTheme.blue
#define UI_RED        activeTheme.red
#define UI_ORANGE     activeTheme.orange
#define UI_CLOUD      activeTheme.cloud
#define UI_CLOUD_DARK activeTheme.cloudDark

// ---------- Layout ----------
static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;
static constexpr int HEADER_H = 26;
static constexpr int FOOTER_H = 36;
static constexpr int CONTENT_Y = HEADER_H;
static constexpr int CONTENT_H = SCREEN_H - HEADER_H - FOOTER_H;

// ---------- Timing ----------
static constexpr uint32_t WEATHER_REFRESH_MS = 10UL * 60UL * 1000UL;
static constexpr uint32_t UI_HEADER_REFRESH_MS = 1000UL;

// ---------- Display ----------
static constexpr uint8_t TFT_BACKLIGHT_PIN = 21;
static constexpr uint32_t BACKLIGHT_TIMEOUT_MS = 3UL * 60UL * 1000UL;  // backlight timeout - 3 min

// ---------- Touch ----------
static constexpr int TOUCH_CS_PIN   = 33;
static constexpr int TOUCH_IRQ_PIN  = 36;
static constexpr int TOUCH_MOSI_PIN = 32;
static constexpr int TOUCH_MISO_PIN = 39;
static constexpr int TOUCH_SCLK_PIN = 25;

static constexpr uint16_t TOUCH_RAW_X_MIN = 250;
static constexpr uint16_t TOUCH_RAW_X_MAX = 3850;
static constexpr uint16_t TOUCH_RAW_Y_MIN = 220;
static constexpr uint16_t TOUCH_RAW_Y_MAX = 3850;
static constexpr bool TOUCH_SWAP_XY = false;
static constexpr bool TOUCH_INVERT_X = false;
static constexpr bool TOUCH_INVERT_Y = false;

// ---------- Weather ----------
static constexpr int HOURLY_COUNT = 8;
static constexpr int DAILY_COUNT  = 6;

// ---------- SD Card ----------
static constexpr int SD_CS_PIN = 5;   // typical for CYD; adjust if needed

// Serial touch debug
static constexpr bool TOUCH_DEBUG = false;
