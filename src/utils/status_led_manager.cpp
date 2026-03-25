#include "status_led_manager.h"

static constexpr uint32_t LED_HEARTBEAT_ON_MS   = 100;
static constexpr uint32_t LED_HEARTBEAT_OFF_MS  = 1900;
static constexpr uint32_t LED_BLINK2_FIRST_MS   = 100;
static constexpr uint32_t LED_BLINK2_GAP_MS     = 150;
static constexpr uint32_t LED_BLINK2_PAUSE_MS   = 1500;
static constexpr uint32_t LED_ERROR_PERIOD_MS   = 200;

StatusLedManager::StatusLedManager(uint8_t ledPin) {
    pin = ledPin;
    currentStatus = LED_STATUS_OFF;
    previousMillis = 0;
    stateStep = 0;
    ledState = false;
    initialized = false;
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
            // Double blink: ON -> gap -> ON -> pause (4 steps)
            if (stateStep == 0 || stateStep == 2) interval = LED_BLINK2_FIRST_MS;
            else if (stateStep == 1) interval = LED_BLINK2_GAP_MS;
            else interval = LED_BLINK2_PAUSE_MS;

            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 4;
                ledState = (stateStep == 0 || stateStep == 2);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

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
