#include "imu_sensor.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../controllers/tank_controller.h"

extern TankController tankController;

// ── Registradores MPU-6500 (compatível com MPU-6050/9250/9255) ────────────────
static constexpr uint8_t REG_SMPLRT_DIV   = 0x19; // Sample Rate Divider
static constexpr uint8_t REG_CONFIG       = 0x1A; // DLPF (filtro passa-baixa)
static constexpr uint8_t REG_GYRO_CONFIG  = 0x1B; // Escala do giroscópio
static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C; // Escala do acelerômetro
static constexpr uint8_t REG_DATA_START   = 0x3B; // Início dos 14 bytes de dados
static constexpr uint8_t REG_PWR_MGMT_1   = 0x6B; // Gerenciamento de energia

// ── Sensibilidades (para a faixa configurada) ─────────────────────────────────
static constexpr float ACCEL_SENS = 8192.0f;  // LSB/g  (±4g)
static constexpr float GYRO_SENS  = 65.5f;    // LSB/°/s (±500°/s)
static constexpr float TEMP_SENS  = 321.0f;   // LSB/°C (MPU-6500 datasheet)
static constexpr float TEMP_OFF   = 21.0f;    // Offset de temperatura (°C)

// ─────────────────────────────────────────────────────────────────────────────

ImuSensor::ImuSensor() : i2c(&Wire), initialized(false), lastUpdateMs(0),
                         errorCount(0), _compReady(false) {}

bool ImuSensor::initialize(TwoWire* wireInstance) {
    if (wireInstance != nullptr) {
        i2c = wireInstance;
    }

    // Testa a comunicação enviando um byte e verificando ACK
    i2c->beginTransmission(Config::IMU_I2C_ADDR);
    if (i2c->endTransmission() != 0) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "MPU-6500 (0x%02X) não detectado no barramento I2C.", Config::IMU_I2C_ADDR);
        initialized = false;
        data.isValid = false;
        return false;
    }

    // Reset de hardware do chip (limpa qualquer estado corrompido após falha I2C)
    writeRegister(REG_PWR_MGMT_1, 0x80); // bit 7 = DEVICE_RESET
    delay(100); // aguarda o chip concluir o reset interno

    // Acorda o sensor (limpa SLEEP) e usa PLL do giroscópio como clock
    if (!writeRegister(REG_PWR_MGMT_1, 0x01)) return false;
    delay(100); // aguarda o PLL estabilizar

    // Taxa de amostragem: 1000 Hz / (1 + 9) = 100 Hz
    if (!writeRegister(REG_SMPLRT_DIV, 0x09))   return false;

    // Filtro passa-baixa: opção 4 ≈ 20 Hz (reduz ruído eletrônico)
    if (!writeRegister(REG_CONFIG, 0x04))        return false;

    // Giroscópio: ±500°/s → 65.5 LSB/°/s
    if (!writeRegister(REG_GYRO_CONFIG, 0x08))   return false;

    // Acelerômetro: ±4g → 8192 LSB/g
    if (!writeRegister(REG_ACCEL_CONFIG, 0x08))  return false;

    lastUpdateMs = millis();
    errorCount   = 0;
    _compReady   = false; // reinicia o filtro complementar
    initialized  = true;
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "MPU-6500 inicializado (±4g / ±500°/s / 100 Hz)");
    return true;
}

void ImuSensor::update() {
    if (!initialized) return;

    unsigned long now = millis();
    if (now - lastUpdateMs < 20) return; // 50 Hz — sincroniza com o loop de controle principal
    float dt = (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;

    readSensorData();
    computeAngles(dt);
}

const Types::ImuData& ImuSensor::getData() const { return data; }
bool ImuSensor::isDataValid() const               { return data.isValid; }
bool ImuSensor::needsReinit() const               { return errorCount >= Config::I2C_SENSOR_ERROR_THRESHOLD; }

// ── I2C helpers ───────────────────────────────────────────────────────────────

bool ImuSensor::writeRegister(uint8_t reg, uint8_t value) {
    i2c->beginTransmission(Config::IMU_I2C_ADDR);
    i2c->write(reg);
    i2c->write(value);
    return i2c->endTransmission() == 0;
}

bool ImuSensor::readRegisters(uint8_t reg, uint8_t count, uint8_t* buf) {
    i2c->beginTransmission(Config::IMU_I2C_ADDR);
    i2c->write(reg);
    if (i2c->endTransmission(false) != 0) return false; // repeated start

    i2c->requestFrom((uint8_t)Config::IMU_I2C_ADDR, count);
    for (uint8_t i = 0; i < count; i++) {
        if (!i2c->available()) return false;
        buf[i] = i2c->read();
    }
    return true;
}

// ── Leitura de dados ──────────────────────────────────────────────────────────

void ImuSensor::readSensorData() {
    uint8_t buf[14];
    if (!readRegisters(REG_DATA_START, 14, buf)) {
        errorCount++;
        if (errorCount == Config::I2C_SENSOR_ERROR_THRESHOLD) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "MPU-6500 (0x%02X) perdeu comunicação após %d erros consecutivos. Dados inválidos — aguardando recuperação.", Config::IMU_I2C_ADDR, Config::I2C_SENSOR_ERROR_THRESHOLD);
            data.isValid = false;
        }
        return;
    }

    // Sucesso, reseta erros
    errorCount = 0;

    // Monta int16 big-endian e converte para unidade física
    auto toInt16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };

    data.accelX = toInt16(buf[0],  buf[1])  / ACCEL_SENS;
    data.accelY = toInt16(buf[2],  buf[3])  / ACCEL_SENS;
    data.accelZ = toInt16(buf[4],  buf[5])  / ACCEL_SENS;

    // Temperatura: TEMP_OUT / 321.0 + 21.0 (MPU-6500 datasheet §4.16)
    data.temperature = toInt16(buf[6], buf[7]) / TEMP_SENS + TEMP_OFF;

    // Giroscópio: converte raw → deg/s
    data.gyroX = toInt16(buf[8],  buf[9])  / GYRO_SENS;
    data.gyroY = toInt16(buf[10], buf[11]) / GYRO_SENS;
    data.gyroZ = toInt16(buf[12], buf[13]) / GYRO_SENS;

    // MPU-6500 não tem magnetômetro — campos mag permanecem em 0.0

    data.isValid    = true;
    data.lastUpdate = millis();
}

// ── Ângulos Euler ─────────────────────────────────────────────────────────────

void ImuSensor::computeAngles(float dt) {
    // Ângulos derivados do acelerômetro (referência absoluta, mas ruidosa em movimento)
    float accelRoll  = atan2f(data.accelY, data.accelZ) * RAD_TO_DEG;
    float accelPitch = atan2f(-data.accelX,
                               sqrtf(data.accelY * data.accelY + data.accelZ * data.accelZ))
                       * RAD_TO_DEG;

    if (!_compReady) {
        // Primeira medição: semeia o filtro diretamente do acelerômetro
        data.roll  = accelRoll;
        data.pitch = accelPitch;
        _compReady = true;
        return;
    }

    // Filtro complementar: gyro corrige curto prazo; accel ancora longo prazo
    data.roll  = COMP_ALPHA * (data.roll  + data.gyroX * dt) + (1.0f - COMP_ALPHA) * accelRoll;
    data.pitch = COMP_ALPHA * (data.pitch + data.gyroY * dt) + (1.0f - COMP_ALPHA) * accelPitch;
}
