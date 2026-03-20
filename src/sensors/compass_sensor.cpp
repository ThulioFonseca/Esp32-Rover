#include "compass_sensor.h"
#include "../config/config.h"
#include "../controllers/tank_controller.h"

extern TankController tankController;

CompassSensor::CompassSensor() : i2c(&Wire), initialized(false), lastReadTime(0), errorCount(0) {}

bool CompassSensor::initialize(TwoWire* wireInstance) {
    if (wireInstance != nullptr) {
        i2c = wireInstance;
    }

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Inicializando Compass HMC5883L (I2C 0x%02X)...", Config::COMPASS_I2C_ADDR);

    // Verifica presença no barramento
    i2c->beginTransmission(Config::COMPASS_I2C_ADDR);
    if (i2c->endTransmission() != 0) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass HMC5883L (0x%02X) não detectado no barramento I2C.", Config::COMPASS_I2C_ADDR);
        initialized = false;
        data.isValid = false;
        return false;
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
    i2c->beginTransmission(Config::COMPASS_I2C_ADDR);
    i2c->write(reg);
    i2c->write(value);
    return i2c->endTransmission() == 0;
}

bool CompassSensor::readRawData(int16_t* x, int16_t* y, int16_t* z) {
    i2c->beginTransmission(Config::COMPASS_I2C_ADDR);
    i2c->write(REG_DATA_X_MSB);
    if (i2c->endTransmission() != 0) return false;

    i2c->requestFrom(Config::COMPASS_I2C_ADDR, (uint8_t)6);
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
    if (!initialized) return;

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
        if (errorCount == SENSOR_ERROR_THRESHOLD) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Compass HMC5883L (0x%02X) perdeu comunicação após %d erros consecutivos. Dados inválidos — aguardando recuperação.", Config::COMPASS_I2C_ADDR, SENSOR_ERROR_THRESHOLD);
            data.isValid = false;
        }
    }
}

const Types::CompassData& CompassSensor::getData() const {
    return data;
}
