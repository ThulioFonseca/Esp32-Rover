#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>
#include "../types/types.h"
#include "../utils/kalman_filter.h"

class TofSensor {
public:
    TofSensor();

    // wireInstance: barramento I2C compartilhado (&Wire).
    // xshutPin: GPIO que controla XSHUT (Pins::TOF_XSHUT = GPIO25).
    bool initialize(TwoWire* wireInstance, uint8_t xshutPin);

    // Não bloqueante — chama sensor.dataReady() antes de ler. Invocar a 50 Hz.
    void update();

    const Types::TofData& getData() const;

    // true quando errorCount >= ERROR_THRESHOLD (sinaliza necessidade de recovery)
    bool needsReinit() const;

    // Toggle XSHUT LOW→HIGH (power cycle HW) + re-inicializa. Chamado por recoverI2CBus().
    bool softReset();

private:
    TwoWire*        i2c;
    VL53L1X         sensor;
    Types::TofData  data;
    bool            initialized;
    uint8_t         _xshutPin;
    uint16_t        errorCount;

    // Q=2.0 (distância muda rápido num rover em movimento)
    // R=15.0 (ruído típico VL53L1X ±1–3mm; suavização moderada)
    KalmanFilter1D  _kalman;

    static constexpr uint16_t ERROR_THRESHOLD = 10; // espelha I2C_SENSOR_ERROR_THRESHOLD

    bool initHardware(); // lógica de init separada para reutilizar em softReset
};
