#ifndef COMPASS_SENSOR_H
#define COMPASS_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../types/types.h"

class CompassSensor {
private:
    TwoWire* i2c;
    Types::CompassData data;

    bool initialized;
    unsigned long lastReadTime;
    const unsigned long READ_INTERVAL_MS = 100; // 10Hz polling rate

    uint8_t errorCount;
    static constexpr uint8_t SENSOR_ERROR_THRESHOLD = 5;

    // HMC5883L I2C Address
    static constexpr uint8_t HMC5883L_ADDRESS = 0x1E; 

    // HMC5883L Register Map
    static constexpr uint8_t REG_CONFIG_A = 0x00;
    static constexpr uint8_t REG_CONFIG_B = 0x01;
    static constexpr uint8_t REG_MODE     = 0x02;
    static constexpr uint8_t REG_DATA_X_MSB = 0x03;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRawData(int16_t* x, int16_t* y, int16_t* z);

public:
    CompassSensor();

    bool initialize(TwoWire* wireInstance = &Wire);
    void update();
    
    const Types::CompassData& getData() const;
};

#endif
