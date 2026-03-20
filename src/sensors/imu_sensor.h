#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../types/types.h"

/**
 * Driver nativo para MPU-6500 (e família MPU-6050/9250/9255).
 *
 * Comunicação direta via Wire.h — sem dependências externas, sem verificação
 * de WHO_AM_I. Lê os 14 bytes de dados brutos do chip (accel + temp + gyro)
 * usando leitura burst a partir do registrador 0x3B.
 *
 * Sensibilidades configuradas:
 *   Acelerômetro: ±4g   → 8192 LSB/g
 *   Giroscópio:  ±500°/s → 65.5 LSB/°/s
 */
class ImuSensor {
public:
    ImuSensor();

    // Inicializa a configuração dos registradores do chip.
    // Retorna false se o chip não responder ao endereço configurado.
    bool initialize(TwoWire* wireInstance = &Wire);

    // Lê novos dados e atualiza os ângulos. Deve ser chamado a ~50 Hz.
    void update();

    // Inicia uma nova calibração e reseta o Yaw. Retorna imediatamente (não bloqueante).
    void startCalibration();

    const Types::ImuData& getData() const;
    bool isDataValid() const;

private:
    TwoWire*          i2c;
    Types::ImuData    data;
    bool              initialized;
    unsigned long     lastUpdateMs;

    // Valores de calibração do giroscópio (bias/zero-rate offset) em deg/s
    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;

    // Estado da calibração não-bloqueante
    bool isCalibrating;
    int  calibrationSamples;
    long calibSumX, calibSumY, calibSumZ;
    static constexpr int CALIBRATION_SAMPLES_NEEDED = 200;

    uint8_t errorCount;
    static constexpr uint8_t SENSOR_ERROR_THRESHOLD = 5;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t count, uint8_t* buf);
    void readSensorData();
    void computeAngles(float dt);
    void processCalibration();
};

#endif
