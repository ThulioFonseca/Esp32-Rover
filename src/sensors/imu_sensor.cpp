#include "imu_sensor.h"
#include "../config/config.h"
#include "../config/pins.h"

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
                         gyroBiasX(0), gyroBiasY(0), gyroBiasZ(0),
                         isCalibrating(false), calibrationSamples(0),
                         calibSumX(0), calibSumY(0), calibSumZ(0),
                         errorCount(0), lastInitAttempt(0) {}

bool ImuSensor::initialize(TwoWire* wireInstance) {
    if (wireInstance != nullptr) {
        i2c = wireInstance;
    }

    if (millis() - lastInitAttempt < INIT_RETRY_INTERVAL_MS) {
        return false;
    }
    lastInitAttempt = millis();

    // Testa a comunicação enviando um byte e verificando ACK
    i2c->beginTransmission(Config::IMU_I2C_ADDR);
    if (i2c->endTransmission() != 0) {
        Serial.println("[ERRO] MPU-6500 não respondeu no barramento I2C");
        initialized = false;
        data.isValid = false;
        return false;
    }

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

    // Inicia a calibração assíncrona do giroscópio para remover o bias (zero-rate offset)
    startCalibration();

    lastUpdateMs = millis();
    errorCount = 0;
    initialized  = true;
    Serial.println("[INFO] MPU-6500 inicializado (±4g / ±500°/s / 100 Hz)");
    return true;
}

void ImuSensor::startCalibration() {
    if (!initialized) return;
    
    Serial.println("[INFO] Iniciando calibração do IMU. Mantenha o rover parado...");
    isCalibrating = true;
    calibrationSamples = 0;
    calibSumX = 0;
    calibSumY = 0;
    calibSumZ = 0;
    data.yaw = 0.0f; // Reseta o heading
}

void ImuSensor::update() {
    if (!initialized) {
        initialize(nullptr);
        return;
    }

    unsigned long now = millis();
    float dt = (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;

    if (isCalibrating) {
        processCalibration();
        return; // Durante a calibração, não atualiza os ângulos normais
    }

    readSensorData();
    computeAngles(dt);
}

const Types::ImuData& ImuSensor::getData() const { return data; }
bool ImuSensor::isDataValid() const               { return data.isValid; }

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
        if (errorCount > 5) {
            Serial.println("[AVISO] MPU-6500 perdeu comunicação. Agendando reinicialização...");
            initialized = false;
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

    // Giroscópio: converte raw → deg/s e aplica compensação do bias
    data.gyroX = (toInt16(buf[8],  buf[9])  / GYRO_SENS) - gyroBiasX;
    data.gyroY = (toInt16(buf[10], buf[11]) / GYRO_SENS) - gyroBiasY;
    data.gyroZ = (toInt16(buf[12], buf[13]) / GYRO_SENS) - gyroBiasZ;

    // MPU-6500 não tem magnetômetro — campos mag permanecem em 0.0

    data.isValid    = true;
    data.lastUpdate = millis();
}

// ── Ângulos Euler ─────────────────────────────────────────────────────────────

void ImuSensor::computeAngles(float dt) {
    // Roll e pitch do acelerômetro (precisos em regime quasi-estático)
    data.roll  = atan2f(data.accelY, data.accelZ) * RAD_TO_DEG;
    data.pitch = atan2f(-data.accelX,
                        sqrtf(data.accelY * data.accelY + data.accelZ * data.accelZ))
                 * RAD_TO_DEG;

    // Aplica deadband (zona morta) no giroscópio Z para evitar drift por micro-ruídos estáticos
    float gz = data.gyroZ;
    if (abs(gz) < 0.5f) { // Se a rotação for menor que 0.5 deg/s, considera como parado
        gz = 0.0f;
    }

    // Yaw integrado do giroscópio Z (sem referência absoluta → drift longo prazo esperado, 
    // mas drift estático de curto prazo agora está mitigado pelo bias+deadband)
    data.yaw += gz * dt;
    if (data.yaw >  180.0f) data.yaw -= 360.0f;
    if (data.yaw < -180.0f) data.yaw += 360.0f;
}

// ── Calibração Assíncrona ─────────────────────────────────────────────────────

void ImuSensor::processCalibration() {
    uint8_t buf[6];
    // Lê apenas os registradores do giroscópio (0x43)
    if (readRegisters(0x43, 6, buf)) {
        // Ignora as primeiras 50 amostras (estabilização térmica/filtros)
        if (calibrationSamples < 50) {
            calibrationSamples++;
            return;
        }

        calibSumX += (int16_t)((buf[0] << 8) | buf[1]);
        calibSumY += (int16_t)((buf[2] << 8) | buf[3]);
        calibSumZ += (int16_t)((buf[4] << 8) | buf[5]);
        
        calibrationSamples++;

        // Verifica se terminou (50 descartadas + CALIBRATION_SAMPLES_NEEDED úteis)
        if (calibrationSamples >= (50 + CALIBRATION_SAMPLES_NEEDED)) {
            gyroBiasX = (calibSumX / (float)CALIBRATION_SAMPLES_NEEDED) / GYRO_SENS;
            gyroBiasY = (calibSumY / (float)CALIBRATION_SAMPLES_NEEDED) / GYRO_SENS;
            gyroBiasZ = (calibSumZ / (float)CALIBRATION_SAMPLES_NEEDED) / GYRO_SENS;
            
            isCalibrating = false;
            
            Serial.print("[INFO] Calibração concluída! Bias Z: ");
            Serial.print(gyroBiasZ, 4);
            Serial.println(" deg/s");
        }
    }
}
