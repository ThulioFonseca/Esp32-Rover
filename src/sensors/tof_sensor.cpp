#include "tof_sensor.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../controllers/tank_controller.h"

extern TankController tankController;

TofSensor::TofSensor()
    : i2c(&Wire), initialized(false), _xshutPin(Pins::TOF_XSHUT),
      errorCount(0), _kalman(2.0f, 15.0f) {}

bool TofSensor::initialize(TwoWire* wireInstance, uint8_t xshutPin) {
    if (wireInstance != nullptr) i2c = wireInstance;
    _xshutPin = xshutPin;

    // Habilita sensor: XSHUT é ativo-LOW, manter HIGH para operação normal
    pinMode(_xshutPin, OUTPUT);
    digitalWrite(_xshutPin, HIGH);
    delay(2); // VL53L1X datasheet: ≥1.2ms após XSHUT↑ antes do I2C estar pronto

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO,
        "Inicializando VL53L1X (I2C 0x%02X, XSHUT GPIO%d)...",
        Config::TOF_I2C_ADDR, _xshutPin);

    return initHardware();
}

bool TofSensor::initHardware() {
    sensor.setBus(i2c);
    sensor.setTimeout(500);

    if (!sensor.init()) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR,
            "VL53L1X (0x%02X) não detectado no barramento I2C.", Config::TOF_I2C_ADDR);
        initialized = false;
        data.isValid = false;
        return false;
    }

    sensor.setDistanceMode(VL53L1X::Long);                         // até 4m
    sensor.setMeasurementTimingBudget(Config::TOF_TIMING_BUDGET_US); // 50ms
    sensor.startContinuous(Config::TOF_CONTINUOUS_PERIOD_MS);       // período 50ms

    errorCount = 0;
    _kalman.reset();
    initialized = true;
    data.isValid = true;

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO,
        "VL53L1X inicializado — modo Long, budget 50ms, periodo 50ms.");
    return true;
}

void TofSensor::update() {
    if (!initialized) return;

    // Não bloqueante: só processa quando uma nova medição está disponível (~20Hz)
    if (!sensor.dataReady()) return;

    uint16_t raw = sensor.read(false); // false = não bloqueia aguardando medição
    uint8_t  status = sensor.ranging_data.range_status;

    // range_status == 0: leitura válida. Outros valores (4=fora de alcance, etc.) = inválido.
    if (status == 0) {
        data.distanceMm = _kalman.update((float)raw);
        data.lastUpdate = millis();
        data.isValid    = true;
        errorCount      = 0;
    } else {
        errorCount++;
        if (errorCount == ERROR_THRESHOLD) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR,
                "VL53L1X (0x%02X) perdeu comunicação após %d erros consecutivos. Aguardando recuperação.",
                Config::TOF_I2C_ADDR, ERROR_THRESHOLD);
            data.isValid = false;
        }
    }
}

const Types::TofData& TofSensor::getData() const {
    return data;
}

bool TofSensor::needsReinit() const {
    return errorCount >= ERROR_THRESHOLD;
}

bool TofSensor::softReset() {
    // Power cycle via XSHUT: LOW → delay → HIGH reinicia o estado interno do chip
    digitalWrite(_xshutPin, LOW);
    delay(10);
    digitalWrite(_xshutPin, HIGH);
    delay(2); // aguarda I2C ficar pronto novamente

    return initHardware();
}
