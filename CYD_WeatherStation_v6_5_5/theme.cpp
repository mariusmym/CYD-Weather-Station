#include "config.h"
#include <Preferences.h>

ThemeColors activeTheme = THEME_DARK;
bool themeDark = true;

static Preferences themePrefs;

void theme_set(bool dark) {
  themeDark = dark;
  activeTheme = dark ? THEME_DARK : THEME_LIGHT;
  themePrefs.begin("theme", false);
  themePrefs.putBool("dark", dark);
  themePrefs.end();
}

// Call once in setup before ui_begin
void theme_load() {
  themePrefs.begin("theme", true);
  themeDark = themePrefs.getBool("dark", true);
  themePrefs.end();
  activeTheme = themeDark ? THEME_DARK : THEME_LIGHT;
}
