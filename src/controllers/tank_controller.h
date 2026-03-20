#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include "../types/types.h"
#include "../communication/channel_manager.h"
#include "../controllers/motor_controller.h"
#include "../sensors/imu_sensor.h"
#include "../sensors/gps_sensor.h"
#include "../sensors/compass_sensor.h"
#include "../debug/debug_manager.h"

struct PendingConfig {
  volatile bool hasChanges = false;
  volatile bool debugEnabled;
  volatile bool darkTheme;
  volatile bool systemArmed;
  volatile bool wifiChange = false;
  uint8_t wifiMode;
  String wifiSSID;
  String wifiPass;
};

class TankController {
private:
  ChannelManager channelManager;
  MotorController motorController;
  ImuSensor imuSensor;
  GpsSensor gpsSensor;
  CompassSensor compassSensor;

  Types::SystemState currentState;
  volatile bool systemArmed;

public:
  DebugManager debugManager;
  
  TankController();

  bool initialize();
  void update(); // Chamado pela TankControlTask a 50 Hz via FreeRTOS.
  void setDebugMode(bool enabled);
  void setSystemArmed(bool armed);

  const Types::ChannelData&   getChannelData()    const;
  const Types::MotorCommands& getMotorCommands()  const;
  const Types::ImuData&       getImuData()        const;
  const Types::GpsData&       getGpsData()        const;
  const Types::CompassData&   getCompassData()    const;
  Types::SystemState          getSystemState()    const;
  bool                        isSystemArmed()     const;

  String getSystemLogs(); // Retorna o buffer circular de logs
  void clearSystemLogs();

private:
  void updateSystem();
  void handleTimeout();
  void processControls();
  void updateState();
};

#endif