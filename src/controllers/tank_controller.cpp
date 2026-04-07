#include "tank_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/gpio.h"

TankController::TankController()
    : currentState(Types::INITIALIZING), systemArmed(false),
      sensorMutex(NULL), i2cConsecutiveErrors(0), recoveryAttempts(0),
      sensorFaultCount(0) {}

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
    Wire.setTimeOut(Config::I2C_NORMAL_TIMEOUT_MS);

    // Aguarda estabilização do barramento e dos sensores após boot/restart.
    // Sensores em estado de brownout podem precisar de até 2s para VDD estabilizar.
    vTaskDelay(pdMS_TO_TICKS(2000));

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
    // systemArmed permanece false — usuário arma via interface web
    debugManager.logf(DebugManager::LOG_LEVEL_INFO, "ESCs inicializados — aguardando armamento via interface");

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

    // Heartbeat de debug a cada 2s — confirma que debug está ativo independente do estado.
    // Útil quando não há sinal RC (TIMEOUT) ou sticks não se movem (sem printControlState).
    if (debugManager.isDebugEnabled()) {
        static unsigned long lastHeartbeat = 0;
        unsigned long now = millis();
        if (now - lastHeartbeat >= 2000) {
            lastHeartbeat = now;
            const char* stateStr;
            switch (currentState) {
                case Types::INITIALIZING: stateStr = "INIT";    break;
                case Types::ARMING:       stateStr = "ARMING";  break;
                case Types::ARMED:        stateStr = "ARMED";   break;
                case Types::TIMEOUT:      stateStr = "TIMEOUT"; break;
                case Types::ERROR:        stateStr = "ERROR";   break;
                default:                  stateStr = "?";       break;
            }
            debugManager.logSerial(DebugManager::LOG_LEVEL_DEBUG,
                "[HB] %s | armed:%d | heap:%u", stateStr, systemArmed, ESP.getFreeHeap());
        }
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

    // 2b. Contar sensores em falha — leitura direta dos objetos (já atualizados neste ciclo)
    uint8_t faults = 0;
    if (Config::IMU_ENABLED     && !imuSensor.getData().isValid)     faults++;
    if (                            !compassSensor.getData().isValid) faults++;
    if (Config::GPS_ENABLED     && !gpsSensor.getData().isValid)     faults++;
    sensorFaultCount = faults;  // escrita atômica (uint8_t, Core 1 exclusivo)

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

uint8_t TankController::getSensorFaultCount() const {
    return sensorFaultCount;
}

String TankController::getSystemLogs() {
    return debugManager.getLogs();
}

void TankController::clearSystemLogs() {
    debugManager.clearLogs();
}

void TankController::recoverI2CBus() {
    debugManager.logf(DebugManager::LOG_LEVEL_WARN,
        "I2C recovery tentativa %d — iniciando sequência de recuperação",
        recoveryAttempts + 1);

    // ── Nível 0: Soft-reset via registrador (Wire ainda ativo) ───────────────
    // Tenta antes de qualquer manipulação de GPIO. Se o barramento ainda aceita
    // writes, o sensor sai do estado inconsistente sem precisar de GPIO bit-bang.
    {
        bool imuSoftOk     = !Config::IMU_ENABLED || imuSensor.softReset();
        bool compassSoftOk = compassSensor.softReset();

        if (imuSoftOk && compassSoftOk) {
            vTaskDelay(pdMS_TO_TICKS(200)); // aguarda reset interno dos sensores

            bool imuOk     = !Config::IMU_ENABLED || imuSensor.initialize(&Wire);
            bool compassOk = compassSensor.initialize(&Wire);

            if (imuOk && compassOk) {
                debugManager.logf(DebugManager::LOG_LEVEL_INFO,
                    "Recovery I2C (soft-reset) bem-sucedido na tentativa %d — sensores online",
                    recoveryAttempts + 1);
                recoveryAttempts = 0;
                Wire.setTimeOut(Config::I2C_NORMAL_TIMEOUT_MS); // restaura timeout normal
                return;
            }
        }
        debugManager.logf(DebugManager::LOG_LEVEL_WARN,
            "Soft-reset insuficiente — escalando para GPIO bit-bang");
    }

    // ── Nível 1: GPIO bit-bang SCL ───────────────────────────────────────────

    // 1. Para o driver Wire
    Wire.end();
    vTaskDelay(pdMS_TO_TICKS(5));

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

    vTaskDelay(pdMS_TO_TICKS(5)); // settling após STOP antes de reinicializar Wire

    // 4. Reinicializa o periférico I2C
    Wire.begin(Pins::SDA, Pins::SCL, Config::I2C_FREQ_HZ);
    Wire.setTimeOut(Config::I2C_RECOVERY_TIMEOUT_MS);

    // 5. Reinicializa sensores e verifica resultado
    bool imuOk     = !Config::IMU_ENABLED || imuSensor.initialize(&Wire);
    bool compassOk = compassSensor.initialize(&Wire);

    if (imuOk && compassOk) {
        debugManager.logf(DebugManager::LOG_LEVEL_INFO,
            "Recovery I2C (bit-bang) bem-sucedido na tentativa %d — sensores online",
            recoveryAttempts + 1);
        recoveryAttempts = 0;
        Wire.setTimeOut(Config::I2C_NORMAL_TIMEOUT_MS); // restaura timeout normal
        return;
    }

    if (!imuOk)     debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "IMU ainda não responde (tentativa %d)", recoveryAttempts + 1);
    if (!compassOk) debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass ainda não responde (tentativa %d)", recoveryAttempts + 1);

    recoveryAttempts++;

    // ── Nível 2: Espera longa — sensores precisam de power cycle ────────────
    // ESP.restart() NÃO corta VDD dos sensores — é ineficaz quando eles estão
    // em estado de brownout. Em vez disso, aguarda 30s sem atividade no barramento
    // para que a tensão se estabilize e os sensores possam se recuperar sozinhos.
    // Se isso ainda falhar, o usuário deve fazer um power cycle manual (desligar USB).
    if (recoveryAttempts >= 3) {
        debugManager.logf(DebugManager::LOG_LEVEL_ERROR,
            "Barramento I2C irrecuperável após %d tentativas de software. "
            "Aguardando 30s sem atividade — se o problema persistir, faça power cycle (desconectar/reconectar USB).",
            recoveryAttempts);
        Wire.setTimeOut(Config::I2C_NORMAL_TIMEOUT_MS); // restaura timeout antes da espera
        vTaskDelay(pdMS_TO_TICKS(30000));
        recoveryAttempts = 0; // permite novas tentativas após a espera
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
