#ifndef PINS_H
#define PINS_H

namespace Pins {
  // ESCs
  constexpr uint8_t ESC_LEFT = 19;
  constexpr uint8_t ESC_RIGHT = 18;
  
  // Sensores (futuro)
  constexpr uint8_t BATTERY_VOLTAGE = 34;
  constexpr uint8_t TEMPERATURE = 35;
  
  // I2C (futuro)
  constexpr uint8_t SDA = 21;
  constexpr uint8_t SCL = 22;
  
  // SPI (futuro)
  constexpr uint8_t MOSI = 23;
  constexpr uint8_t MISO = 19;
  constexpr uint8_t SCK = 18;
  constexpr uint8_t CS = 5;
}

#endif