#include "compass_sensor.h"
#include "../config/pins.h"
#include "../controllers/tank_controller.h"

extern TankController tankController;

CompassSensor::CompassSensor() : i2c(&Wire), initialized(false), lastReadTime(0), errorCount(0), lastInitAttempt(0) {}

// Força os pinos de I2C do Compass para recriar comunicação
void CompassSensor::resetI2CBus() {
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_WARN, "Executando I2C Bus Clear Agressivo no Compass...");
    
    // Desabilita a interface Wire para liberar os pinos
    if(i2c) i2c->end();

    // Espera a linha baixar as tensoes
    delay(10);

    // Configura os pinos como saída
    pinMode(Pins::COMPASS_SDA, OUTPUT);
    pinMode(Pins::COMPASS_SCL, OUTPUT);

    // Força tudo para HIGH primeiro
    digitalWrite(Pins::COMPASS_SDA, HIGH);
    digitalWrite(Pins::COMPASS_SCL, HIGH);
    delay(10);

    // Alterna o Clock 9 vezes de forma LENTA (10ms de ciclo) para dar tempo aos sensores 
    for (int i = 0; i < 9; i++) {
        digitalWrite(Pins::COMPASS_SCL, LOW);
        delay(5);
        digitalWrite(Pins::COMPASS_SCL, HIGH);
        delay(5);
    }
    
    // Envia um START condition manual (SDA vai pra LOW enquanto SCL tá HIGH)
    digitalWrite(Pins::COMPASS_SDA, LOW);
    delay(5);
    // Envia um STOP condition manual (SCL pra HIGH, e SDA transita pra HIGH)
    digitalWrite(Pins::COMPASS_SCL, HIGH);
    delay(5);
    digitalWrite(Pins::COMPASS_SDA, HIGH);
    delay(10);

    // Reinicializa a Wire com o novo clock mega lento (10kHz)
    if(i2c) i2c->begin(Pins::COMPASS_SDA, Pins::COMPASS_SCL, 10000);
    delay(50); // Dá um tempo pro hardware interno do ESP32 acordar
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "I2C Bus Clear concluído.");
}

bool CompassSensor::initialize(TwoWire* wireInstance) {
    if (wireInstance != nullptr) {
        i2c = wireInstance;
    }
    
    // Evita spam de inicialização se estiver em loop de recuperação
    if (millis() - lastInitAttempt < INIT_RETRY_INTERVAL_MS) {
        return false;
    }
    lastInitAttempt = millis();

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "(Re)Inicializando Compass HMC5883L (Manual I2C)...");
    
    // Pequeno delay para estabilização do barramento
    delay(50);

    // Verifica presença no barramento
    i2c->beginTransmission(HMC5883L_ADDRESS);
    if (i2c->endTransmission() != 0) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass HMC5883L não encontrado no barramento I2C");
        
        // Tenta destravar o I2C Bus
        resetI2CBus();
        
        // Tenta encontrar novamente
        i2c->beginTransmission(HMC5883L_ADDRESS);
        if (i2c->endTransmission() != 0) {
             tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass HMC5883L falhou mesmo após I2C Reset.");
             initialized = false;
             data.isValid = false;
             return false;
        }
    }

    // Configura o chip:
    // Config Register A: 0x70 = 8 samples averaged, 15Hz default
    writeRegister(REG_CONFIG_A, 0x70);
    // Config Register B: 0xA0 = Gain 5 (default)
    writeRegister(REG_CONFIG_B, 0xA0);
    // Mode Register: 0x00 = Continuous-Measurement Mode
    writeRegister(REG_MODE, 0x00);

    errorCount = 0;
    initialized = true;
    data.isValid = true;
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Compass HMC5883L inicializado com sucesso.");
    return true;
}

bool CompassSensor::writeRegister(uint8_t reg, uint8_t value) {
    i2c->beginTransmission(HMC5883L_ADDRESS);
    i2c->write(reg);
    i2c->write(value);
    return i2c->endTransmission() == 0;
}

bool CompassSensor::readRawData(int16_t* x, int16_t* y, int16_t* z) {
    i2c->beginTransmission(HMC5883L_ADDRESS);
    i2c->write(REG_DATA_X_MSB);
    if (i2c->endTransmission() != 0) return false;

    i2c->requestFrom(HMC5883L_ADDRESS, (uint8_t)6);
    if (i2c->available() >= 6) {
        // HMC5883L armazena X, Z, Y (nessa ordem!)
        *x = (int16_t)(i2c->read() << 8 | i2c->read());
        *z = (int16_t)(i2c->read() << 8 | i2c->read());
        *y = (int16_t)(i2c->read() << 8 | i2c->read());
        return true;
    }
    return false;
}

void CompassSensor::update() {
    if (!initialized) {
        // Tenta reconectar periodicamente em background
        initialize(nullptr);
        return;
    }

    // Rate limiting para não sobrecarregar o barramento (10Hz)
    if (millis() - lastReadTime < READ_INTERVAL_MS) {
        return;
    }
    lastReadTime = millis();

    int16_t rawX, rawY, rawZ;
    if (readRawData(&rawX, &rawY, &rawZ)) {
        // Sensibilidade para Gain 5: 0.92 mG/LSB (aprox 1090 LSB/Gauss)
        // Convertendo para micro-Tesla (1 Gauss = 100 uT)
        data.x = (float)rawX / 10.9f;
        data.y = (float)rawY / 10.9f;
        data.z = (float)rawZ / 10.9f;

        // Cálculo do Heading (bússola) em radianos
        float headingRad = atan2(data.y, data.x);
        
        // Correção de declinação magnética (opcional, setado em 0 aqui)
        float declinationAngle = 0.0f;
        headingRad += declinationAngle;

        // Ajuste de ângulo para 0-2PI
        if (headingRad < 0) headingRad += 2 * PI;
        if (headingRad > 2 * PI) headingRad -= 2 * PI;

        data.heading = headingRad * 180.0f / M_PI;
        data.lastUpdate = millis();
        data.isValid = true;
        errorCount = 0; // Reset errors on success
    } else {
        errorCount++;
        if (errorCount > 5) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_WARN, "Compass HMC5883L perdeu comunicação. Agendando reinicialização...");
            initialized = false;
            data.isValid = false;
        }
    }
}

const Types::CompassData& CompassSensor::getData() const {
    return data;
}
