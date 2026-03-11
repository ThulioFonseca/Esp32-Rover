#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include "../types/types.h"
#include "../communication/channel_manager.h"
#include "../controllers/motor_controller.h"
#include "../sensors/imu_sensor.h"
#include "../sensors/gps_sensor.h"
#include "../debug/debug_manager.h"

class TankController {
private:
  ChannelManager channelManager;
  MotorController motorController;
  ImuSensor imuSensor;
  GpsSensor gpsSensor;
  DebugManager debugManager;

  Types::SystemState currentState;
  bool systemArmed;

public:
  TankController();

  bool initialize();
  void update(); // Chamado pela TankControlTask a 50 Hz via FreeRTOS.
  void setDebugMode(bool enabled);
  void setSystemArmed(bool armed);

  const Types::ChannelData&   getChannelData()    const;
  const Types::MotorCommands& getMotorCommands()  const;
  const Types::ImuData&       getImuData()        const;
  const Types::GpsData&       getGpsData()        const;
  Types::SystemState          getSystemState()    const;
  bool                        isSystemArmed()     const;

  void calibrateImu(); // Dispara calibração assíncrona

private:
  void updateSystem();
  void handleTimeout();
  void processControls();
  void updateState();
};

#endif