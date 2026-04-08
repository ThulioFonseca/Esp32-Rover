#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace Pins {
  // ESCs
  constexpr uint8_t ESC_LEFT = 19;
  constexpr uint8_t ESC_RIGHT = 18;
  
  // Comunicação (IBUS)
  constexpr uint8_t IBUS_RX = 16;
  constexpr uint8_t IBUS_TX = 17;

  // Sensores (futuro)
  constexpr uint8_t BATTERY_VOLTAGE = 34;
  constexpr uint8_t TEMPERATURE = 35;
  
  // I2C — barramento único compartilhado (IMU + Compass + TOF + sensores futuros)
  constexpr uint8_t SDA = 21;
  constexpr uint8_t SCL = 22;

  // TOF (VL53L1X) — GPIO 25 (livre, não conflita com ESC nem SPI)
  // XSHUT é ativo-LOW: HIGH = sensor habilitado, LOW = power-down/reset HW
  constexpr uint8_t TOF_XSHUT = 25;
  
  // SPI (futuro)
  // ATENÇÃO: GPIO 18 e 19 conflitam com ESC_RIGHT e ESC_LEFT!
  // Ao usar SPI, os ESCs devem ser realocados para outros pinos.
  // Sugestão de pinos livres para ESCs: GPIO 25 e 26.
  constexpr uint8_t SPI_MOSI = 23;
  constexpr uint8_t SPI_MISO = 12; // Movido de 19 (conflito com ESC_LEFT)
  constexpr uint8_t SPI_SCK  = 14; // Movido de 18 (conflito com ESC_RIGHT)
  constexpr uint8_t SPI_CS   = 5;

  // Serial 1 (GPS Neo-7M/M8N)
  constexpr uint8_t PIN_GPS_RX = 4;
  constexpr uint8_t PIN_GPS_TX = 5; // Nota: Compartilhado com SPI_CS, use com cautela se usar SPI futuramente

}

#endif
