#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

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
  constexpr unsigned long IBUS_TIMEOUT_MS = 400;
  constexpr unsigned long CONTROL_INTERVAL_MS = 20;
  constexpr unsigned long DEBUG_INTERVAL_MS = 100;
  constexpr unsigned long ARMING_TIME_MS = 1500;
  
  // Configurações de comunicação
  constexpr unsigned long SERIAL_BAUD = 115200;
  constexpr uint8_t IBUS_UART = 2;
  constexpr uint8_t IBUS_MODE = 1;
}

#endif