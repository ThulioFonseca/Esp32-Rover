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
  constexpr unsigned long CONTROL_INTERVAL_MS   = 20;   // 50 Hz — frequência do loop de controle
  constexpr unsigned long WS_BROADCAST_INTERVAL_MS = 100; // 10 Hz — frequência do broadcast WebSocket
  constexpr unsigned long DEBUG_INTERVAL_MS     = 100;
  constexpr unsigned long ARMING_TIME_MS        = 1500;
  constexpr int           TANK_MUTEX_TIMEOUT_MS = 5;    // Timeout do try-lock no tankControlTask

  // WebSocket
  constexpr size_t WS_BINARY_FRAME_SIZE = 121; // Tamanho fixo do frame binário (ver broadcastSensorData)

  // Configurações de Rede (Persistentes via NVS)
  extern uint8_t WIFI_MODE; // 0 = AP, 1 = STA
  extern String STA_SSID;
  extern String STA_PASS;

  // Função para carregar preferências do NVS
  void loadPreferences();

  // Salva todos os valores atuais de Config:: no NVS em uma única transação.
  // Deve ser chamada apenas em contexto seguro (ex: sequência de reboot com tasks pausadas).
  void savePreferences();

  // Theme
  extern bool DARK_THEME;

  // Debug
  extern bool DEBUG_ENABLED;
  
  // Comunicação iBUS (Serial2)
  constexpr unsigned long SERIAL_BAUD = 115200;
  constexpr uint8_t IBUS_MODE = 1; // IBusBM: modo 1 = apenas leitura

  // I2C — barramento único compartilhado (pinos 21/22)
  // Para Fast Mode (400 kHz) com pull-ups de 2.2 kΩ, trocar para 400000.
  constexpr uint32_t I2C_FREQ_HZ = 100000; // Standard Mode (100 kHz)
  constexpr uint16_t I2C_SENSOR_ERROR_THRESHOLD = 5; // Erros consecutivos antes de recovery

  // IMU (MPU-9250 / 6500 / 9255 via I2C)
  // AD0 = LOW  → 0x68 | AD0 = HIGH → 0x69
  constexpr uint8_t IMU_I2C_ADDR = 0x68; // AD0 = LOW (GND)
  extern bool IMU_ENABLED;

  // Compass (HMC5883L via I2C)
  constexpr uint8_t COMPASS_I2C_ADDR = 0x1E;

  // GPS (Neo-7M / M8N)
  constexpr unsigned long GPS_BAUD = 9600;
  constexpr int8_t GPS_TIMEZONE_OFFSET_HOURS = -3; // GMT-3 (Brasília)
  extern bool GPS_ENABLED;
}

#endif
