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
  
  // Zona morta do stick (µs ao redor do centro 1500)
  // Jitter típico de RC é 15-30µs — valor baixo demais causa drift de controle
  constexpr int STICK_DEADZONE = 25;
  
  // Timeouts e intervalos
  extern unsigned long IBUS_TIMEOUT_MS;
  constexpr unsigned long CONTROL_INTERVAL_MS   = 20;   // 50 Hz — frequência do loop de controle
  constexpr unsigned long WS_BROADCAST_INTERVAL_MS = 50;  // 20 Hz — frequência do broadcast WebSocket
  constexpr unsigned long DEBUG_INTERVAL_MS     = 100;
  constexpr unsigned long ARMING_TIME_MS        = 1500;
  constexpr int           TANK_MUTEX_TIMEOUT_MS = 5;    // Timeout do try-lock no tankControlTask

  // WebSocket
  constexpr size_t WS_BINARY_FRAME_SIZE = 127; // Tamanho fixo do frame binário (126 dados + 1 CRC8; +5 bytes TOF)
  constexpr uint8_t MAX_WS_CLIENTS = 4;        // Limite de clientes simultâneos (~2-4KB heap cada)

  // Configurações de Rede (Persistentes via NVS)
  extern uint8_t WIFI_MODE; // 0 = AP, 1 = STA
  extern String STA_SSID;
  extern String STA_PASS;

  // Nomes e cores dos canais iBUS (Persistentes via NVS)
  constexpr int CHANNEL_COUNT = 10;
  extern String CHANNEL_NAMES[CHANNEL_COUNT];   // ex: "CH 1" … "CH 10" (max 20 chars)
  extern String CHANNEL_COLORS[CHANNEL_COUNT];  // id de cor: "1"…"15" (índice chart-color)

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
  constexpr uint32_t I2C_FREQ_HZ = 100000; // Standard Mode (100 kHz) — mais robusto a variações de VDD e temperatura; latência aceitável para sensores de navegação
  constexpr uint16_t I2C_SENSOR_ERROR_THRESHOLD = 10; // Erros consecutivos antes de recovery (margem para glitches EMI)

  // Timeouts do barramento I2C por transação
  constexpr int I2C_NORMAL_TIMEOUT_MS   = 50; // Operação normal — margem para sensores lentos
  constexpr int I2C_RECOVERY_TIMEOUT_MS = 20; // Durante recovery — encerra transações travadas mais rápido

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

  // TOF (VL53L1X via I2C)
  // Endereço 0x29 — sem conflito com IMU (0x68) ou Compass (0x1E)
  constexpr uint8_t  TOF_I2C_ADDR             = 0x29;
  constexpr uint32_t TOF_TIMING_BUDGET_US     = 50000; // 50ms → ~20Hz de leituras
  constexpr uint16_t TOF_CONTINUOUS_PERIOD_MS = 50;    // período entre medições (ms)
  constexpr float    TOF_MAX_RANGE_MM         = 4000.0f; // alcance máximo modo Long
  extern bool TOF_ENABLED;
}

#endif
