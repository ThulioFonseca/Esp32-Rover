#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

class WebServerManager {
public:
    WebServerManager(const char* ssid = "ESP32-AP", const char* password = "esp32pass");
    
    // Inicializa o Wi-Fi como AP e o servidor Async
    bool begin();

private:
    const char* ap_ssid;
    const char* ap_password;
    AsyncWebServer server;

    void setupRoutes();
};

#endif // WEB_SERVER_MANAGER_H
