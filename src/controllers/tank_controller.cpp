#include "tank_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>
#include <Wire.h>

TankController::TankController()
    : currentState(Types::INITIALIZING), systemArmed(false) {}

bool TankController::initialize() {
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "=== Inicializando TankController ===");

    currentState = Types::INITIALIZING;

    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando ChannelManager...");
    if (!channelManager.initialize()) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao inicializar ChannelManager");
        currentState = Types::ERROR;
        return false;
    }

    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando MotorController...");
    if (!motorController.initialize()) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao inicializar MotorController");
        currentState = Types::ERROR;
        return false;
    }

    // Barramento I2C único compartilhado — IMU, Compass e sensores futuros
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando I2C SDA=%d SCL=%d @ %lu Hz", Pins::SDA, Pins::SCL, Config::I2C_FREQ_HZ);
    Wire.begin(Pins::SDA, Pins::SCL, Config::I2C_FREQ_HZ);
    Wire.setTimeOut(20); // 20ms limite por transação I2C (padrão: 50ms)

    // Sensores são opcionais — falha na inicialização não trava o boot.
    if (Config::IMU_ENABLED) {
        debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando ImuSensor...");
        if (!imuSensor.initialize(&Wire)) {
            debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "ImuSensor (MPU-6500, I2C 0x%02X) não detectado — desabilitado até próximo reboot.", Config::IMU_I2C_ADDR);
        } else {
            debugManager.logf(DebugManager::LOG_LEVEL_INFO, "ImuSensor inicializado.");
        }
    }

    if (Config::GPS_ENABLED) {
        debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando GpsSensor...");
        if (!gpsSensor.initialize()) {
            debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "GpsSensor não inicializado — verifique conexão UART.");
        }

        debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando CompassSensor...");
        if (!compassSensor.initialize(&Wire)) {
            debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "CompassSensor (HMC5883L, I2C 0x1E) não detectado — desabilitado até próximo reboot.");
        }
    }

    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Iniciando sequência de armamento...");
    currentState = Types::ARMING;
    motorController.performArmingSequence();

    currentState = Types::ARMED;
    systemArmed = true;
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Armamento concluído — sistema ARMADO");

    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "=== TankController inicializado com sucesso ===");
    return true;
}

void TankController::update() {
    channelManager.update();
    updateState();

    // Sensores movidos para sensorUpdateTask — leituras I2C sem segurar o tankMutex.

    switch (currentState) {
        case Types::ARMED:   updateSystem();               break;
        case Types::TIMEOUT: handleTimeout();              break;
        case Types::ERROR:   motorController.setNeutral(); break;
        default:                                           break;
    }
}

void TankController::setDebugMode(bool enabled) {
    debugManager.setEnabled(enabled);
}

void TankController::setSystemArmed(bool armed) {
    systemArmed = armed;
    if (!systemArmed) {
        motorController.setNeutral();
    }
}

void TankController::updateSensors() {
    if (Config::IMU_ENABLED) imuSensor.update();
    if (Config::GPS_ENABLED) {
        gpsSensor.update();
        compassSensor.update();
    }
}

const Types::ChannelData& TankController::getChannelData() const {
    return channelManager.getChannelData();
}

const Types::MotorCommands& TankController::getMotorCommands() const {
    return motorController.getCommands();
}

const Types::ImuData& TankController::getImuData() const {
    return imuSensor.getData();
}

const Types::GpsData& TankController::getGpsData() const {
    return gpsSensor.getData();
}

const Types::CompassData& TankController::getCompassData() const {
    return compassSensor.getData();
}

Types::SystemState TankController::getSystemState() const {
    return currentState;
}

bool TankController::isSystemArmed() const {
    return systemArmed;
}

String TankController::getSystemLogs() {
    return debugManager.getLogs();
}

void TankController::clearSystemLogs() {
    debugManager.clearLogs();
}

void TankController::updateSystem() {
    if (!channelManager.isDataValid()) return;

    processControls();

    debugManager.printControlState(channelManager.getChannelData(), motorController.getCommands());
}

void TankController::handleTimeout() {
    motorController.setNeutral();
    // Recuperação do estado TIMEOUT → ARMED é tratada exclusivamente em updateState().
}

void TankController::processControls() {
    const Types::ChannelData& channels = channelManager.getChannelData();

    if (systemArmed) {
        motorController.update(channels.nThrottle, channels.nSteering);
    } else {
        motorController.setNeutral();
    }
}

void TankController::updateState() {
    Types::SystemState previousState = currentState;

    if (currentState == Types::ARMED && channelManager.hasTimeout()) {
        currentState = Types::TIMEOUT;
    } else if (currentState == Types::TIMEOUT && !channelManager.hasTimeout() && channelManager.isDataValid()) {
        currentState = Types::ARMED;
    }

    if (previousState != currentState) {
        debugManager.printSystemStatus(currentState);
    }
}
