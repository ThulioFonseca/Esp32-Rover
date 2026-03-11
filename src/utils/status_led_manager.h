#ifndef STATUS_LED_MANAGER_H
#define STATUS_LED_MANAGER_H

#include <Arduino.h>

enum LedStatus {
    LED_STATUS_OFF,
    LED_STATUS_OPERATIONAL, // Heartbeat (curto, pausa longa)
    LED_STATUS_WARNING,     // Duas piscadas curtas, pausa
    LED_STATUS_ERROR        // Pisca rápido contínuo
};

class StatusLedManager {
private:
    uint8_t pin;
    LedStatus currentStatus;
    unsigned long previousMillis;
    int stateStep;
    bool ledState;

public:
    StatusLedManager(uint8_t ledPin);
    void begin();
    void setStatus(LedStatus status);
    void update();
};

#endif
