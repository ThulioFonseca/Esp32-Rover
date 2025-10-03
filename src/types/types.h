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
    CH_AUX7 = 8,
    CH_AUX8 = 9,
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
    float temperature;
    float gyroZ;
    bool isValid;
    unsigned long lastUpdate;
    
    SensorData();
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