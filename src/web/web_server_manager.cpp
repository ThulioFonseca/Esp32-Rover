#include "web_server_manager.h"
#include "web_pages.h"
#include <ArduinoJson.h>

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
    // Servir arquivos embarcados na memória (PROGMEM) para evitar conflitos com SPIFFS/Interrupts
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/css", style_css);
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "application/javascript", script_js);
    });

    // Health check
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
    });

    // System Info API
    server.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["chip_model"] = ESP.getChipModel();
        doc["chip_revision"] = ESP.getChipRevision();
        doc["free_heap"] = ESP.getFreeHeap();
        doc["uptime"] = millis();
        doc["ip"] = WiFi.softAPIP().toString();
        doc["ssid"] = WiFi.softAPSSID();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Not found
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });
}
