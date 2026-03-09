#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

// Arduino.h deve ser incluído antes de MPU9250.h para garantir que as macros
// PI, DEG_TO_RAD, byte e Serial já estejam definidas quando a biblioteca é compilada.
#include <Arduino.h>
#include <MPU9250.h>
#include "../types/types.h"

class ImuSensor {
public:
    ImuSensor();

    // Inicializa I2C e detecta o MPU. Retorna false se o sensor não for encontrado.
    bool initialize();

    // Lê novos dados do sensor. Deve ser chamado periodicamente (50–100 Hz).
    void update();

    // Executa calibração de acelerômetro e giroscópio em repouso (~5 s).
    // Chamar apenas uma vez, com o rover parado e nivelado.
    void calibrate();

    const Types::ImuData& getData() const;
    bool isDataValid() const;

private:
    MPU9250 mpu;
    Types::ImuData data;
    bool initialized;

    void readSensorData();
};

#endif
