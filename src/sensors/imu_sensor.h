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

    const Types::ImuData& getData() const;
    bool isDataValid() const;

private:
    TwoWire*          i2c;
    Types::ImuData    data;
    bool              initialized;
    unsigned long     lastUpdateMs;

    uint8_t errorCount;
    static constexpr uint8_t SENSOR_ERROR_THRESHOLD = 5;

    // Filtro complementar roll/pitch (gyro + acelerômetro)
    bool  _compReady;                          // false até a primeira medição
    static constexpr float COMP_ALPHA = 0.95f; // α: 0.95 → τ ≈ 380 ms a 50 Hz

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t count, uint8_t* buf);
    void readSensorData();
    void computeAngles(float dt);
};

#endif
