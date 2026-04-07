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
#include <atomic>

struct PendingConfig {
  volatile bool hasChanges = false;
  volatile bool debugEnabled;
  volatile bool darkTheme;
  volatile bool systemArmed;
  volatile bool wifiChange = false;
  uint8_t wifiMode;
  char wifiSSID[33];  // max 32-char SSID + null — char[] avoids heap-allocated String across tasks
  char wifiPass[65];  // max 64-char passphrase + null
  volatile bool channelConfigChange = false;
  char channelNames[10][21];  // max 20 chars + null por canal
  char channelColors[10][4];  // id "1"–"15" + null por canal
};

class TankController {
private:
  ChannelManager channelManager;
  MotorController motorController;
  ImuSensor imuSensor;
  GpsSensor gpsSensor;
  CompassSensor compassSensor;

  Types::SystemState currentState;
  std::atomic<bool> systemArmed;

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
  static constexpr uint16_t I2C_RECOVERY_THRESHOLD = 10; // ~200ms a 50Hz antes de tentar recovery
  void recoverI2CBus();

  // ERROR state recovery
  unsigned long errorRecoveryTimer;
  uint8_t       errorRecoveryAttempts;
  static constexpr unsigned long ERROR_RECOVERY_INTERVAL_MS = 10000; // 10s entre tentativas
  static constexpr uint8_t      ERROR_MAX_RECOVERY_ATTEMPTS = 3;
  void attemptErrorRecovery();

  // Sensor fault counting (for LED indication)
  volatile uint8_t sensorFaultCount;

  template<typename T>
  T snapshotUnderMutex(const T& src) const {
      T copy{};
      // 5ms timeout: sensorMutex is held only for a struct copy (microseconds),
      // so this effectively never waits while still being race-free.
      if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
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
  uint8_t                     getSensorFaultCount() const;

  String getSystemLogs(); // Retorna o buffer circular de logs
  void clearSystemLogs();

private:
  void updateSystem();
  void handleTimeout();
  void processControls();
  void updateState();
};

#endif