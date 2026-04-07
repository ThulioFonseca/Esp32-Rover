#include "gps_sensor.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../controllers/tank_controller.h"

extern TankController tankController;

GpsSensor::GpsSensor() : gpsSerial(1), lastValidTime(0) {}

bool GpsSensor::initialize() {
    // Configura a porta Serial1 (UART1) para o GPS usando os pinos RX=4 e TX=5
    gpsSerial.begin(Config::GPS_BAUD, SERIAL_8N1, Pins::PIN_GPS_RX, Pins::PIN_GPS_TX);
    
    // TinyGPSPlus doesn't require "initialization" per se, it just processes characters.
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "GPS_Sensor inicializado na Serial1 (RX=%d, TX=%d)", Pins::PIN_GPS_RX, Pins::PIN_GPS_TX);
    
    data.isValid = false;
    return true;
}

void GpsSensor::update() {
    bool newData = false;
    
    // Leitura não-bloqueante: limita bytes por chamada para não bloquear o loop de controle
    for (int i = 0; i < 64 && gpsSerial.available() > 0; i++) {
        if (gps.encode(gpsSerial.read())) {
            newData = true;
        }
    }

    if (newData) {
        // Atualiza campos individuais quando disponíveis
        if (gps.location.isValid()) {
            data.latitude  = gps.location.lat();
            data.longitude = gps.location.lng();
        }

        if (gps.altitude.isValid()) {
            data.altitude = gps.altitude.meters();
        }

        if (gps.speed.isValid()) {
            data.speed = gps.speed.kmph();
        }

        if (gps.course.isValid()) {
            data.course = gps.course.deg();
        }

        if (gps.satellites.isValid()) {
            data.satellites = gps.satellites.value();
        }

        if (gps.hdop.isValid()) {
            data.hdop = gps.hdop.value();
        }

        if (gps.time.isValid()) {
            int8_t tzOffset = Config::GPS_TIMEZONE_OFFSET_HOURS;
            int hour = (int)gps.time.hour() + tzOffset;
            if (hour < 0)  hour += 24;
            if (hour > 23) hour -= 24;
            data.timeHour   = (uint8_t)hour;
            data.timeMinute = gps.time.minute();
            data.timeSecond = gps.time.second();
        }
        if (gps.date.isValid() && gps.time.isValid()) {
            formatDateTime(data.dateTime, sizeof(data.dateTime));
        }

        // Validação de qualidade composta: exige localização + satélites + HDOP aceitável
        // HDOP raw do TinyGPSPlus é value()*100, então 500 = HDOP 5.0
        bool hasLocation   = gps.location.isValid();
        bool hasEnoughSats = gps.satellites.isValid() && data.satellites >= 4;
        bool hasGoodHdop   = gps.hdop.isValid() && data.hdop < 500;

        if (hasLocation && hasEnoughSats && hasGoodHdop) {
            data.isValid = true;
            lastValidTime = millis();
        } else if (hasLocation) {
            // Localização disponível mas qualidade insuficiente — mantém dados mas não marca válido
            // Permite que a UI mostre posição aproximada sem confiar nela para navegação
            lastValidTime = millis();
        }

        data.lastUpdate = millis();
    }

    // Invalida se sem fix de qualidade por mais de 3 segundos
    if (data.isValid && (millis() - lastValidTime > 3000)) {
        data.isValid = false;
    }
}

void GpsSensor::formatDateTime(char* buf, size_t bufLen) {
    // Formata no padrão ISO 8601 com timezone configurável via Config::GPS_TIMEZONE_OFFSET_HOURS
    int hour = gps.time.hour() + Config::GPS_TIMEZONE_OFFSET_HOURS;
    if (hour < 0)  hour += 24;
    if (hour > 23) hour -= 24;

    snprintf(buf, bufLen, "%04d-%02d-%02dT%02d:%02d:%02d%+03d:00",
             gps.date.year(), gps.date.month(), gps.date.day(),
             hour, gps.time.minute(), gps.time.second(),
             (int)Config::GPS_TIMEZONE_OFFSET_HOURS);
}

const Types::GpsData& GpsSensor::getData() const {
    return data;
}
