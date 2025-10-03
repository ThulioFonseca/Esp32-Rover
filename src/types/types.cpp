#include "types.h"
#include "../config/config.h"

namespace Types {
  ChannelData::ChannelData() 
    : throttle(Config::PWM_MID), 
      steering(Config::PWM_MID),
      nThrottle(0.0f), 
      nSteering(0.0f),
      isValid(false), 
      lastUpdateTime(0) {
    for (int i = 0; i < 8; i++) {
      aux[i] = Config::PWM_MID;
    }
  }

  MotorCommands::MotorCommands() 
    : left(0.0f), 
      right(0.0f), 
      leftPWM(Config::ESC_NEUTRAL), 
      rightPWM(Config::ESC_NEUTRAL) {}

  SensorData::SensorData() 
    : batteryVoltage(0.0f), 
      temperature(0.0f), 
      gyroZ(0.0f),
      isValid(false), 
      lastUpdate(0) {}
}