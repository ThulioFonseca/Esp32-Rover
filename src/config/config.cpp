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

  String CHANNEL_NAMES[CHANNEL_COUNT]  = {"CH 1","CH 2","CH 3","CH 4","CH 5","CH 6","CH 7","CH 8","CH 9","CH 10"};
  String CHANNEL_COLORS[CHANNEL_COUNT] = {"1","2","3","4","5","6","7","8","9","10"};

  Preferences preferences;

  void loadPreferences() {
    preferences.begin("rover", true); // true = apenas leitura
    WIFI_MODE = preferences.getUChar("wifi_mode", 0);
    if (WIFI_MODE > 1) WIFI_MODE = 0; // apenas AP(0) ou STA(1) são válidos

    IBUS_TIMEOUT_MS = preferences.getULong("ibus_timeout", 1000);
    if (IBUS_TIMEOUT_MS < 100 || IBUS_TIMEOUT_MS > 5000) IBUS_TIMEOUT_MS = 1000;

    STA_SSID = preferences.getString("sta_ssid", "");
    STA_PASS = preferences.getString("sta_pass", "");
    DEBUG_ENABLED = preferences.getBool("debug", false);
    DARK_THEME = preferences.getBool("dark_theme", true);
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      char keyN[6], keyC[6];
      snprintf(keyN, sizeof(keyN), "ch%dn", i);
      snprintf(keyC, sizeof(keyC), "ch%dc", i);
      CHANNEL_NAMES[i]  = preferences.getString(keyN, CHANNEL_NAMES[i]);
      CHANNEL_COLORS[i] = preferences.getString(keyC, CHANNEL_COLORS[i]);
    }
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
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      char keyN[6], keyC[6];
      snprintf(keyN, sizeof(keyN), "ch%dn", i);
      snprintf(keyC, sizeof(keyC), "ch%dc", i);
      preferences.putString(keyN, CHANNEL_NAMES[i]);
      preferences.putString(keyC, CHANNEL_COLORS[i]);
    }
    preferences.end();
  }
}
