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

    // WebSocket: envia snapshot de dados para todos os clientes conectados.
    // Chamado pela wsBroadcastTask no Core 0 a cada 50ms (20 Hz).
    void broadcastSensorData();

    // Retorna a quantidade de clientes WebSocket conectados.
    size_t wsClientCount() const;

private:
    const char* ap_ssid;
    const char* ap_password;
    AsyncWebServer server;
    AsyncWebSocket ws;

    void setupRoutes();
    void setupWebSocket();
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);
};

#endif // WEB_SERVER_MANAGER_H
