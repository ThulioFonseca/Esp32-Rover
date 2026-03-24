#include "config.h"
#include <Preferences.h>

namespace Config {
  unsigned long IBUS_TIMEOUT_MS = 1000;
  bool DARK_THEME = true;
  bool DEBUG_ENABLED = false;
  bool IMU_ENABLED = true; // Controla se o sensor IMU deve ser inicializado
  bool GPS_ENABLED = true; // Controla se o GPS deve ser inicializado

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

  void savePreferences() {
    // Grava todos os valores atuais em uma única transação NVS.
    // Deve ser chamada apenas em contexto seguro (tasks Core 1 estacionadas).
    preferences.begin("rover", false);
    preferences.putBool("debug",      DEBUG_ENABLED);
    preferences.putBool("dark_theme", DARK_THEME);
    preferences.putUChar("wifi_mode", WIFI_MODE);
    preferences.putString("sta_ssid", STA_SSID);
    preferences.putString("sta_pass", STA_PASS);
    preferences.end();
  }
}
