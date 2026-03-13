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
    : batteryVoltage(0.0f), isValid(false), lastUpdate(0) {}

  ImuData::ImuData()
    : accelX(0.0f), accelY(0.0f), accelZ(0.0f),
      gyroX(0.0f),  gyroY(0.0f),  gyroZ(0.0f),
      magX(0.0f),   magY(0.0f),   magZ(0.0f),
      roll(0.0f), pitch(0.0f), yaw(0.0f),
      temperature(0.0f),
      isValid(false), lastUpdate(0) {}

  CompassData::CompassData()
    : x(0.0f), y(0.0f), z(0.0f), heading(0.0f),
      isValid(false), lastUpdate(0) {}

  GpsData::GpsData()
    : latitude(0.0f), longitude(0.0f), altitude(0.0f),
      speed(0.0f), course(0.0f), satellites(0), hdop(0.0f),
      dateTime(""), isValid(false), lastUpdate(0) {}
}
