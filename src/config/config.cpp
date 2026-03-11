#include "config.h"
#include <Preferences.h>

namespace Config {
  unsigned long IBUS_TIMEOUT_MS = 1000;
  bool DARK_THEME = true;
  bool DEBUG_ENABLED = false;
  bool IMU_ENABLED = true; // Controla se o sensor IMU deve ser inicializado

  uint8_t WIFI_MODE = 0; // 0 = AP, 1 = STA
  String STA_SSID = "";
  String STA_PASS = "";

  Preferences preferences;

  void loadPreferences() {
    preferences.begin("rover", true); // true = apenas leitura
    WIFI_MODE = preferences.getUChar("wifi_mode", 0);
    STA_SSID = preferences.getString("sta_ssid", "");
    STA_PASS = preferences.getString("sta_pass", "");
    DEBUG_ENABLED = preferences.getBool("debug", false);
    DARK_THEME = preferences.getBool("dark_theme", true);
    preferences.end();
  }

  void saveNetworkPreferences(uint8_t mode, const String& ssid, const String& pass) {
    preferences.begin("rover", false); // false = leitura/escrita
    preferences.putUChar("wifi_mode", mode);
    preferences.putString("sta_ssid", ssid);
    preferences.putString("sta_pass", pass);
    preferences.end();
    
    WIFI_MODE = mode;
    STA_SSID = ssid;
    STA_PASS = pass;
  }

  void saveDebugPreference(bool enabled) {
    preferences.begin("rover", false);
    preferences.putBool("debug", enabled);
    preferences.end();
    
    DEBUG_ENABLED = enabled;
  }

  void saveThemePreference(bool dark_theme) {
    preferences.begin("rover", false);
    preferences.putBool("dark_theme", dark_theme);
    preferences.end();
    
    DARK_THEME = dark_theme;
  }
}
