#include "display_manager.h"
#include "config.h"

TFT_eSPI tft = TFT_eSPI();

void display_begin() {
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true); //because the colors were inverted
  tft.fillScreen(UI_BG_0);
  tft.setTextColor(UI_TEXT_MAIN, UI_BG_0);
  tft.setTextFont(2);
}
