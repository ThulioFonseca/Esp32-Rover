#include "web_server_manager.h"

WebServerManager::WebServerManager(const char* ssid, const char* password)
    : ap_ssid(ssid), ap_password(password), server(80) {}

bool WebServerManager::begin() {
    Serial.println("[INFO] Iniciando modo Access Point...");

    if (!WiFi.softAP(ap_ssid, ap_password)) {
        Serial.println("[ERRO] Falha ao iniciar Access Point!");
        return false;
    }

    Serial.print("[INFO] AP iniciado: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("[INFO] IP do AP: ");
    Serial.println(WiFi.softAPIP());

    setupRoutes();

    server.begin();
    Serial.println("[INFO] Servidor Async iniciado na porta 80");

    return true;
}

void WebServerManager::setupRoutes() {
    // Health check
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
    });

    // Not found
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });
}
