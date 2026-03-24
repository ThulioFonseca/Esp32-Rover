#ifndef COMPASS_SENSOR_H
#define COMPASS_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../types/types.h"
#include "../utils/kalman_filter.h"

class CompassSensor {
private:
    TwoWire* i2c;
    Types::CompassData data;

    bool initialized;
    unsigned long lastReadTime;
    const unsigned long READ_INTERVAL_MS = 100; // 10Hz polling rate

    uint16_t errorCount;

    // Kalman 1D para suavização do heading (Q=0.001, R=0.5)
    KalmanFilter1D _headingKalman;

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
    bool needsReinit() const;
};

#endif
