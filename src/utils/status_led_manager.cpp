#include "status_led_manager.h"

StatusLedManager::StatusLedManager(uint8_t ledPin) {
    pin = ledPin;
    currentStatus = LED_STATUS_OFF;
    previousMillis = 0;
    stateStep = 0;
    ledState = false;
}

void StatusLedManager::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
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
    if (currentStatus == LED_STATUS_OFF) return;

    unsigned long currentMillis = millis();
    unsigned long interval = 0;

    switch (currentStatus) {
        case LED_STATUS_OPERATIONAL:
            // Heartbeat: 100ms ON -> 1900ms OFF
            interval = (stateStep == 0) ? 100 : 1900;
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 2;
                ledState = (stateStep == 0);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

        case LED_STATUS_WARNING:
            // Double blink: 100ms ON -> 150ms OFF -> 100ms ON -> 1500ms OFF
            if (stateStep == 0 || stateStep == 2) interval = 100;
            else if (stateStep == 1) interval = 150;
            else interval = 1500;

            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                stateStep = (stateStep + 1) % 4;
                ledState = (stateStep == 0 || stateStep == 2);
                digitalWrite(pin, ledState ? HIGH : LOW);
            }
            break;

        case LED_STATUS_ERROR:
            // Fast continuous blink: 200ms ON -> 200ms OFF
            interval = 200;
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
