#include "tank_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>
#include <Wire.h>

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

    // Inicialização dos barramentos I2C
    Serial.println("[INFO] Inicializando I2C 0 (IMU) nos pinos SDA=" + String(Pins::SDA) + ", SCL=" + String(Pins::SCL));
    Wire.begin(Pins::SDA, Pins::SCL, Config::IMU_I2C_FREQ_HZ);
    
    Serial.println("[INFO] Inicializando I2C 1 (Compass) nos pinos SDA=" + String(Pins::COMPASS_SDA) + ", SCL=" + String(Pins::COMPASS_SCL));
    pinMode(Pins::COMPASS_SDA, INPUT_PULLUP);
    pinMode(Pins::COMPASS_SCL, INPUT_PULLUP);
    // Usa uma frequência de relógio extremamente lenta (10 kHz) para compensar a ausência de resistores de pull-up físicos fortes.
    Wire1.begin(Pins::COMPASS_SDA, Pins::COMPASS_SCL, 10000);

    // Sensores são opcionais e têm auto-recuperação, falha inicial não trava o boot.
    if (Config::IMU_ENABLED) {
        Serial.println("[INFO] Inicializando ImuSensor...");
        if (!imuSensor.initialize(&Wire)) {
            Serial.println("[AVISO] ImuSensor não encontrado no boot. Tentará reconectar em background.");
        } else {
            Serial.println("[INFO] ImuSensor inicializado — inciando calibração automática...");
            imuSensor.startCalibration();
        }
    }

    if (Config::GPS_ENABLED) {
        Serial.println("[INFO] Inicializando GpsSensor...");
        if (!gpsSensor.initialize()) {
            Serial.println("[AVISO] GpsSensor não inicializado. Tentará reconectar em background.");
        }
        
        Serial.println("[INFO] Inicializando CompassSensor...");
        if (!compassSensor.initialize(&Wire1)) {
            Serial.println("[AVISO] CompassSensor não encontrado no boot. Tentará reconectar em background.");
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

    // Sensores atualizam independentes do estado do sistema
    imuSensor.update();
    gpsSensor.update();
    compassSensor.update();

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
