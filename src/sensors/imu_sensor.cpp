#include "imu_sensor.h"
#include "../config/config.h"
#include "../config/pins.h"
#include <Wire.h>
#include <Arduino.h>

ImuSensor::ImuSensor() : initialized(false) {}

bool ImuSensor::initialize() {
    // Inicializa o barramento I2C com os pinos e frequência definidos em config/pins.h.
    Wire.begin(Pins::SDA, Pins::SCL, Config::IMU_I2C_FREQ_HZ);

    if (!mpu.setup(Config::IMU_I2C_ADDR, MPU9250Setting(), Wire)) {
        Serial.println("[ERRO] IMU não detectado no endereço I2C configurado");
        return false;
    }

    initialized = true;
    return true;
}

void ImuSensor::update() {
    if (!initialized) return;

    if (mpu.update()) {
        readSensorData();
    }
}

/**
 * Executa calibração de acelerômetro e giroscópio.
 * O rover deve estar parado e nivelado durante o processo (~5 s).
 * Os offsets são armazenados internamente pela biblioteca e aplicados
 * automaticamente nas leituras subsequentes.
 */
void ImuSensor::calibrate() {
    if (!initialized) return;

    Serial.println("[IMU] Iniciando calibração — mantenha o rover nivelado e parado...");
    mpu.calibrateAccelGyro();
    Serial.println("[IMU] Calibração de acelerômetro e giroscópio concluída");
}

const Types::ImuData& ImuSensor::getData() const {
    return data;
}

bool ImuSensor::isDataValid() const {
    return data.isValid;
}

void ImuSensor::readSensorData() {
    data.accelX = mpu.getAccX();
    data.accelY = mpu.getAccY();
    data.accelZ = mpu.getAccZ();

    data.gyroX = mpu.getGyroX();
    data.gyroY = mpu.getGyroY();
    data.gyroZ = mpu.getGyroZ();

    // Magnetômetro disponível apenas no MPU-9250/9255.
    // No MPU-6500 (sem mag), a biblioteca retorna 0.0.
    data.magX = mpu.getMagX();
    data.magY = mpu.getMagY();
    data.magZ = mpu.getMagZ();

    // Ângulos Euler calculados pelo filtro de fusão Mahony interno da biblioteca.
    data.roll  = mpu.getRoll();
    data.pitch = mpu.getPitch();
    data.yaw   = mpu.getYaw();

    data.temperature = mpu.getTemperature();

    data.isValid    = true;
    data.lastUpdate = millis();
}
