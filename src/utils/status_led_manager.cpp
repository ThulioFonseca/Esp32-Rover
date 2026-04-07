#include "status_led_manager.h"

// Heartbeat (STA mode)
static constexpr uint32_t LED_HEARTBEAT_ON_MS   = 100;
static constexpr uint32_t LED_HEARTBEAT_OFF_MS  = 1900;

// AP mode — piscada lenta
static constexpr uint32_t LED_AP_ON_MS   = 500;
static constexpr uint32_t LED_AP_OFF_MS  = 2000;

// Sensor faults — N piscadas rápidas
static constexpr uint32_t LED_FAULT_ON_MS    = 100;
static constexpr uint32_t LED_FAULT_GAP_MS   = 150;
static constexpr uint32_t LED_FAULT_PAUSE_MS = 1500;

// Error state
static constexpr uint32_t LED_ERROR_PERIOD_MS   = 200;

StatusLedManager::StatusLedManager(uint8_t ledPin) {
    pin = ledPin;
    currentStatus = LED_STATUS_OFF;
    previousMillis = 0;
    stateStep = 0;
    ledState = false;
    initialized = false;
    faultBlinkCount = 0;
}

void StatusLedManager::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    initialized = true;
}

void StatusLedManager::setStatus(LedStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        stateStep = 0;
        previousMillis = millis();
        if (status == LED_STATUS_OFF) {
            ledState = false;
            digitalWrite(pin, LOW);
        }
    }
}

void StatusLedManager::setFaultCount(uint8_t count) {
    if (currentStatus != LED_STATUS_FAULT || faultBlinkCount != count) {
        currentStatus = LED_STATUS_FAULT;
        faultBlinkCount = count;
        stateStep = 0;
        previousMillis = millis();
    }
}

void StatusLedManager::update() {
    if (!initialized) return;
    if (currentStatus == LED_STATUS_OFF) return;

    unsigned long currentMillis = millis();
    unsigned long interval = 0;

    switch (currentStatus) {
        case LED_STATUS_OPERATIONAL:
            // Heartbeat: LED_HEARTBEAT_ON_MS ON -> LED_HEARTBEAT_OFF_MS OFF
            interval = (stateStep == 0) ? LED_HEARTBEAT_ON_MS : LED_HEARTBEAT_OFF_MS;
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 2;
                ledState = (stateStep == 0);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

        case LED_STATUS_WARNING:
            // AP mode — slow single blink: 500ms ON -> 2000ms OFF
            interval = (stateStep == 0) ? LED_AP_ON_MS : LED_AP_OFF_MS;
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 2;
                ledState = (stateStep == 0);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

        case LED_STATUS_FAULT: {
            // N blinks: totalSteps = 2*N (pairs ON/OFF, last OFF is pause)
            uint8_t totalSteps = faultBlinkCount * 2;
            bool isLastStep = (stateStep == totalSteps - 1);

            // ON phases (even steps): LED_FAULT_ON_MS
            // OFF phases (odd steps): gap (150ms) except last OFF is pause (1500ms)
            interval = (stateStep % 2 == 0) ? LED_FAULT_ON_MS
                     : (isLastStep           ? LED_FAULT_PAUSE_MS
                                             : LED_FAULT_GAP_MS);

            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % totalSteps;
                ledState = (stateStep % 2 == 0);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;
        }

        case LED_STATUS_ERROR:
            // Fast continuous blink
            interval = LED_ERROR_PERIOD_MS;
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 2;
                ledState = (stateStep == 0);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

        default:
            break;
    }
}
