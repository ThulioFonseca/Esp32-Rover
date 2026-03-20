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
    
    // Leitura não-bloqueante: consome todos os caracteres disponíveis no buffer serial
    while (gpsSerial.available() > 0) {
        if (gps.encode(gpsSerial.read())) {
            newData = true;
        }
    }

    if (newData) {
        // Se a localização é válida
        if (gps.location.isValid()) {
            data.latitude = gps.location.lat();
            data.longitude = gps.location.lng();
            data.isValid = true;
            lastValidTime = millis();
        }

        // Se a altitude é válida
        if (gps.altitude.isValid()) {
            data.altitude = gps.altitude.meters();
        }

        // Se a velocidade é válida
        if (gps.speed.isValid()) {
            data.speed = gps.speed.kmph();
        }

        // Se o curso (heading) é válido
        if (gps.course.isValid()) {
            data.course = gps.course.deg();
        }

        // Satélites
        if (gps.satellites.isValid()) {
            data.satellites = gps.satellites.value();
        }

        // HDOP
        if (gps.hdop.isValid()) {
            data.hdop = gps.hdop.value();
        }

        // Data e hora
        if (gps.date.isValid() && gps.time.isValid()) {
            data.dateTime = getFormattedDateTime();
        }

        data.lastUpdate = millis();
    }
    
    // Invalida o GPS se não tivermos informações válidas de localização nos últimos 3 segundos
    if (data.isValid && (millis() - lastValidTime > 3000)) {
        data.isValid = false;
    }
}

String GpsSensor::getFormattedDateTime() {
    char dateTimeBuffer[30];
    // Formata no padrão ISO 8601 com timezone configurável via Config::GPS_TIMEZONE_OFFSET_HOURS
    int hour = gps.time.hour() + Config::GPS_TIMEZONE_OFFSET_HOURS;
    if (hour < 0)  hour += 24;
    if (hour > 23) hour -= 24;

    // Constrói sufixo "+HH:00" ou "-HH:00" dinamicamente
    char tzSuffix[7];
    snprintf(tzSuffix, sizeof(tzSuffix), "%+03d:00", (int)Config::GPS_TIMEZONE_OFFSET_HOURS);

    snprintf(dateTimeBuffer, sizeof(dateTimeBuffer), "%04d-%02d-%02dT%02d:%02d:%02d%s",
             gps.date.year(), gps.date.month(), gps.date.day(),
             hour, gps.time.minute(), gps.time.second(), tzSuffix);

    return String(dateTimeBuffer);
}

const Types::GpsData& GpsSensor::getData() const {
    return data;
}
