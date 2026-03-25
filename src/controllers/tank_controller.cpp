#include "tank_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/gpio.h"

TankController::TankController()
    : currentState(Types::INITIALIZING), systemArmed(false),
      sensorMutex(NULL), i2cConsecutiveErrors(0), recoveryAttempts(0) {}

bool TankController::initialize() {
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "=== Inicializando TankController ===");

    currentState = Types::INITIALIZING;

    // Mutex para snapshots de sensores — protege apenas cópias rápidas, nunca I2C.
    sensorMutex = xSemaphoreCreateMutex();
    if (sensorMutex == NULL) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao criar sensorMutex");
        return false;
    }

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
    }

    // Compass compartilha o barramento I2C com o IMU — inicializado independentemente do GPS
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando CompassSensor...");
    if (!compassSensor.initialize(&Wire)) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "CompassSensor (HMC5883L, I2C 0x1E) não detectado — desabilitado até próximo reboot.");
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
    // 1. Leituras I2C/UART lentas — SEM mutex (podem demorar até 20ms com timeout)
    if (Config::IMU_ENABLED) imuSensor.update();
    if (Config::GPS_ENABLED) gpsSensor.update();
    compassSensor.update(); // Compass usa I2C independentemente do GPS

    // 2. Cópia atômica para snapshots — mutex breve (microsegundos)
    if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, 0) == pdTRUE) {
        imuSnapshot     = imuSensor.getData();
        gpsSnapshot     = gpsSensor.getData();
        compassSnapshot = compassSensor.getData();
        xSemaphoreGive(sensorMutex);
    }

    // 3. Verifica saúde do barramento I2C — recovery se necessário
    bool imuFailing     = Config::IMU_ENABLED && imuSensor.needsReinit();
    bool compassFailing = compassSensor.needsReinit();

    if (imuFailing || compassFailing) {
        i2cConsecutiveErrors++;
        if (i2cConsecutiveErrors >= I2C_RECOVERY_THRESHOLD) {
            recoverI2CBus();
            i2cConsecutiveErrors = 0;
        }
    } else {
        i2cConsecutiveErrors = 0;
    }
}

const Types::ChannelData& TankController::getChannelData() const {
    return channelManager.getChannelData();
}

const Types::MotorCommands& TankController::getMotorCommands() const {
    return motorController.getCommands();
}

Types::ImuData     TankController::getImuData()     const { return snapshotUnderMutex(imuSnapshot); }
Types::GpsData     TankController::getGpsData()     const { return snapshotUnderMutex(gpsSnapshot); }
Types::CompassData TankController::getCompassData() const { return snapshotUnderMutex(compassSnapshot); }

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

void TankController::recoverI2CBus() {
    debugManager.logf(DebugManager::LOG_LEVEL_WARN,
        "I2C recovery tentativa %d — liberando GPIO matrix, bit-bang SCL, reinit sensores",
        recoveryAttempts + 1);

    // 1. Para o driver Wire
    Wire.end();
    delay(5);

    // 2. Remove SDA e SCL da GPIO matrix do ESP32.
    //    Wire.end() para o driver mas NÃO libera os pinos do controle do periférico
    //    I2C na GPIO matrix. Sem isso, os pulsos do bit-bang brigam com o periférico
    //    que ainda controla os pinos — tornando o processo ineficaz.
    gpio_reset_pin((gpio_num_t)Pins::SDA);
    gpio_reset_pin((gpio_num_t)Pins::SCL);
    delayMicroseconds(100); // estabilização como inputs flutuantes

    // 3. Só bit-bang se SDA estiver realmente preso baixo
    pinMode(Pins::SDA, INPUT_PULLUP);
    pinMode(Pins::SCL, INPUT_PULLUP);
    delayMicroseconds(50);

    if (digitalRead(Pins::SDA) == LOW) {
        debugManager.logf(DebugManager::LOG_LEVEL_WARN, "SDA preso LOW — 9 ciclos SCL para liberar");
        pinMode(Pins::SCL, OUTPUT);
        for (int i = 0; i < 9; i++) {
            digitalWrite(Pins::SCL, HIGH);
            delayMicroseconds(5);
            digitalWrite(Pins::SCL, LOW);
            delayMicroseconds(5);
        }
        // Condição STOP: SCL HIGH → SDA low→high
        pinMode(Pins::SDA, OUTPUT);
        digitalWrite(Pins::SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(Pins::SDA, LOW);
        delayMicroseconds(5);
        digitalWrite(Pins::SDA, HIGH);
        delayMicroseconds(5);
    } else {
        debugManager.logf(DebugManager::LOG_LEVEL_INFO,
            "SDA HIGH após gpio_reset — barramento pode estar livre, reinit Wire");
    }

    delay(5); // settling após STOP antes de reinicializar Wire

    // 4. Reinicializa o periférico I2C
    Wire.begin(Pins::SDA, Pins::SCL, Config::I2C_FREQ_HZ);
    Wire.setTimeOut(20);

    // 5. Reinicializa sensores e verifica resultado
    bool imuOk     = !Config::IMU_ENABLED || imuSensor.initialize(&Wire);
    bool compassOk = !Config::GPS_ENABLED || compassSensor.initialize(&Wire);

    if (imuOk && compassOk) {
        debugManager.logf(DebugManager::LOG_LEVEL_INFO,
            "Recovery I2C bem-sucedido na tentativa %d — sensores online", recoveryAttempts + 1);
        recoveryAttempts = 0;
        return;
    }

    if (!imuOk)     debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "IMU ainda não responde (tentativa %d)", recoveryAttempts + 1);
    if (!compassOk) debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass ainda não responde (tentativa %d)", recoveryAttempts + 1);

    recoveryAttempts++;

    // 6. Último recurso: ESP.restart() após 3 tentativas consecutivas sem sucesso.
    //    Power cycle sempre funciona; ESP.restart() faz reset completo de hardware equivalente.
    //    pendingNvsRtc.magic NÃO é setado — restart é puramente para recovery de hardware.
    if (recoveryAttempts >= 3) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR,
            "Barramento I2C irrecuperável após %d tentativas — ESP.restart()", recoveryAttempts);
        delay(200);
        ESP.restart();
    }
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
