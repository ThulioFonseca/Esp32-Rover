#include "tank_controller.h"
#include "../config/config.h"
#include "../utils/utils.h"
#include <Arduino.h>

TankController::TankController()
    : currentState(Types::INITIALIZING), systemArmed(false) {}

bool TankController::initialize() {
    Serial.println("=== Inicializando TankController ===");

    currentState = Types::INITIALIZING;

    debugManager.initialize();
    debugManager.setEnabled(Config::DEBUG_ENABLED);

    Serial.println("[INFO] Inicializando ChannelManager...");
    if (!channelManager.initialize()) {
        Serial.println("[ERRO] Falha ao inicializar ChannelManager");
        currentState = Types::ERROR;
        return false;
    }

    Serial.println("[INFO] Inicializando MotorController...");
    if (!motorController.initialize()) {
        Serial.println("[ERRO] Falha ao inicializar MotorController");
        currentState = Types::ERROR;
        return false;
    }

    // IMU é opcional: falha na inicialização gera aviso mas não impede o boot.
    if (Config::IMU_ENABLED) {
        Serial.println("[INFO] Inicializando ImuSensor...");
        if (!imuSensor.initialize()) {
            Serial.println("[AVISO] ImuSensor não inicializado — continuando sem IMU");
        }
    }

    Serial.println("[INFO] Iniciando sequência de armamento...");
    currentState = Types::ARMING;
    motorController.performArmingSequence();

    currentState = Types::ARMED;
    systemArmed = true;
    Serial.println("[INFO] Armamento concluído — sistema ARMADO");

    Serial.println("=== TankController inicializado com sucesso ===");
    return true;
}

void TankController::update() {
    channelManager.update();
    updateState();

    // IMU atualiza independente do estado do sistema (dados sempre disponíveis para monitoramento).
    imuSensor.update();

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

const Types::ChannelData& TankController::getChannelData() const {
    return channelManager.getChannelData();
}

const Types::MotorCommands& TankController::getMotorCommands() const {
    return motorController.getCommands();
}

const Types::ImuData& TankController::getImuData() const {
    return imuSensor.getData();
}

Types::SystemState TankController::getSystemState() const {
    return currentState;
}

bool TankController::isSystemArmed() const {
    return systemArmed;
}

void TankController::calibrateImu() {
    imuSensor.startCalibration();
}

void TankController::updateSystem() {
    if (!channelManager.isDataValid()) return;

    processControls();

    if (debugManager.isDebugEnabled()) {
        debugManager.printChannelData(channelManager.getChannelData());
        debugManager.printMotorCommands(motorController.getCommands());
    }
}

void TankController::handleTimeout() {
    motorController.setNeutral();

    if (debugManager.isDebugEnabled()) {
        debugManager.printTimeout();
    }

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

    if (previousState != currentState && debugManager.isDebugEnabled()) {
        debugManager.printSystemStatus(currentState);
    }
}
