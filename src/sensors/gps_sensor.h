#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "../types/types.h"

class GpsSensor {
private:
    HardwareSerial gpsSerial;
    TinyGPSPlus gps;
    Types::GpsData data;
    
    unsigned long lastValidTime;

    void formatDateTime(char* buf, size_t bufLen);

public:
    GpsSensor();

    bool initialize();
    void update();
    
    const Types::GpsData& getData() const;
};

#endif
