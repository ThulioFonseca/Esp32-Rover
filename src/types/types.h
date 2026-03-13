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

  struct SensorData {
    float batteryVoltage;
    bool isValid;
    unsigned long lastUpdate;

    SensorData();
  };

  struct ImuData {
    // Acelerômetro (g)
    float accelX, accelY, accelZ;

    // Giroscópio (deg/s)
    float gyroX, gyroY, gyroZ;

    // Magnetômetro (µT) — zero se o módulo não tiver magnetômetro (ex: MPU-6500)
    float magX, magY, magZ;

    // Ângulos Euler calculados pelo filtro de fusão interno (graus)
    float roll, pitch, yaw;

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
    String dateTime; // formato ISO 8601 com timezone (ex: 2026-12-01T15:30:00-03:00)

    bool isValid;
    unsigned long lastUpdate;

    GpsData();
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