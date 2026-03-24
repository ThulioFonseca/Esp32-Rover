#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include "../types/types.h"
#include "../communication/channel_manager.h"
#include "../controllers/motor_controller.h"
#include "../sensors/imu_sensor.h"
#include "../sensors/gps_sensor.h"
#include "../sensors/compass_sensor.h"
#include "../debug/debug_manager.h"
#include "freertos/semphr.h"

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

  // Mutex que protege os snapshots de sensores — nunca seguro durante I2C.
  // sensorUpdateTask (Core 1) escreve snapshots após leitura I2C.
  // broadcastSensorData (Core 0) e /api/sensors leem dos snapshots.
  SemaphoreHandle_t sensorMutex;

  // Snapshots thread-safe dos dados de sensores (cópia rápida sob mutex)
  Types::ImuData     imuSnapshot;
  Types::GpsData     gpsSnapshot;
  Types::CompassData compassSnapshot;

  // I2C bus recovery
  uint16_t i2cConsecutiveErrors;
  uint8_t  recoveryAttempts;
  static constexpr uint16_t I2C_RECOVERY_THRESHOLD = 50;
  void recoverI2CBus();

  template<typename T>
  T snapshotUnderMutex(const T& src) const {
      T copy;
      if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, 0) == pdTRUE) {
          copy = src;
          xSemaphoreGive(sensorMutex);
      }
      return copy;
  }

public:
  DebugManager debugManager;

  TankController();

  bool initialize();
  void update();        // Chamado pela tankControlTask a 50 Hz — motor control apenas.
  void updateSensors(); // Chamado pela sensorUpdateTask — leituras I2C fora do loop de controle.
  void setDebugMode(bool enabled);
  void setSystemArmed(bool armed);

  const Types::ChannelData&   getChannelData()    const;
  const Types::MotorCommands& getMotorCommands()  const;

  // Getters thread-safe: copiam do snapshot protegido por sensorMutex.
  Types::ImuData     getImuData()     const;
  Types::GpsData     getGpsData()     const;
  Types::CompassData getCompassData() const;

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