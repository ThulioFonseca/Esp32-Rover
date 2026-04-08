#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

namespace Types {
  enum Channels {
    CH_STEERING = 0,
    CH_THROTTLE = 1,
    CH_AUX1 = 2,
    CH_AUX2 = 3,
    CH_AUX3 = 4,
    CH_AUX4 = 5,
    CH_AUX5 = 6,
    CH_AUX6 = 7,
    CH_VRA = 8,
    CH_VRB = 9,
    MAX_CHANNELS = 10
  };

  struct ChannelData {
    int throttle;
    int steering;
    int aux[8];
    float nThrottle;
    float nSteering;
    bool isValid;
    unsigned long lastUpdateTime;
    
    ChannelData();
  };

  struct MotorCommands {
    float left;
    float right;
    int leftPWM;
    int rightPWM;
    
    MotorCommands();
  };

  struct ImuData {
    // Acelerômetro (g)
    float accelX, accelY, accelZ;

    // Giroscópio (deg/s)
    float gyroX, gyroY, gyroZ;

    // Ângulos Euler (graus) — yaw removido (MPU-6500 não tem magnetômetro)
    float roll, pitch;

    float temperature; // °C

    bool isValid;
    unsigned long lastUpdate;

    ImuData();
  };

  struct CompassData {
    float x, y, z;
    float heading; // graus (0 a 360)
    
    bool isValid;
    unsigned long lastUpdate;

    CompassData();
  };

  struct GpsData {
    float latitude;
    float longitude;
    float altitude; // metros
    float speed;    // km/h
    float course;   // graus
    uint32_t satellites;
    float hdop;
    char dateTime[30];        // formato ISO 8601 (buffer fixo, sem alocação heap)
    uint8_t timeHour;         // hora UTC (0-23) — incluído no frame binário
    uint8_t timeMinute;       // minuto (0-59)
    uint8_t timeSecond;       // segundo (0-59)

    bool isValid;
    unsigned long lastUpdate;

    GpsData();
  };

  struct TofData {
    float distanceMm;    // distância filtrada pelo Kalman (mm)
    bool  isValid;       // false até a primeira leitura válida ou após ERROR_THRESHOLD erros
    unsigned long lastUpdate;

    TofData();
  };

  enum SystemState {
    INITIALIZING,
    ARMING,
    ARMED,
    TIMEOUT,
    ERROR
  };
}

#endif