#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <Arduino.h>

namespace Config {
  // Limites PWM
  constexpr int PWM_MIN = 1000;
  constexpr int PWM_MAX = 2000;
  constexpr int PWM_MID = 1500;
  
  // Deadband do ESC
  constexpr int ESC_DEAD_LOW = 1482;
  constexpr int ESC_DEAD_HIGH = 1582;
  constexpr int ESC_NEUTRAL = (ESC_DEAD_LOW + ESC_DEAD_HIGH) / 2;
  
  // Zona morta do stick
  constexpr int STICK_DEADZONE = 10;
  
  // Timeouts e intervalos
  extern unsigned long IBUS_TIMEOUT_MS;
  constexpr unsigned long CONTROL_INTERVAL_MS = 20; // Frequência de controle fixa
  constexpr unsigned long DEBUG_INTERVAL_MS = 100;
  constexpr unsigned long ARMING_TIME_MS = 1500;

  // Configurações de Rede (Persistentes via NVS)
  extern uint8_t WIFI_MODE; // 0 = AP, 1 = STA
  extern String STA_SSID;
  extern String STA_PASS;

  // Função para carregar preferências do NVS
  void loadPreferences();
  void saveNetworkPreferences(uint8_t mode, const String& ssid, const String& pass);
  void saveDebugPreference(bool enabled);

  // Debug
  extern bool DEBUG_ENABLED;
  
  // Comunicação iBUS (Serial2)
  constexpr unsigned long SERIAL_BAUD = 115200;
  constexpr uint8_t IBUS_MODE = 1; // IBusBM: modo 1 = apenas leitura

  // IMU (MPU-9250 / 6500 / 9255 via I2C)
  // AD0 = LOW  → 0x68 | AD0 = HIGH → 0x69
  constexpr uint8_t IMU_I2C_ADDR     = 0x68; // AD0 = LOW (GND)
  constexpr uint32_t IMU_I2C_FREQ_HZ = 100000; // Standard Mode (100 kHz) — mais estável em protoboard
  extern bool IMU_ENABLED;
}

#endif