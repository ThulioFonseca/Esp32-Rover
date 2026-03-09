#include "web_server_manager.h"
#include "web_pages.h"
#include "controllers/tank_controller.h"
#include "config/config.h"
#include <ArduinoJson.h>
#include "freertos/semphr.h"

extern TankController tankController;

// Mutex definido em main.cpp — protege o tankController contra acesso simultâneo
// entre Core 0 (web callbacks) e Core 1 (TankControlTask).
extern SemaphoreHandle_t tankMutex;

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

    // RC Channels API
    server.on("/api/channels", HTTP_GET, [](AsyncWebServerRequest *request){
        // Copia os dados para variáveis locais dentro da região crítica,
        // evitando uso de referência após liberar o mutex.
        Types::ChannelData snapshot;
        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            snapshot = tankController.getChannelData();
            xSemaphoreGive(tankMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
            return;
        }

        JsonDocument doc;
        doc["throttle"] = snapshot.throttle;
        doc["steering"] = snapshot.steering;
        doc["valid"]    = snapshot.isValid;

        JsonArray aux = doc["aux"].to<JsonArray>();
        for(int i = 0; i < 8; i++) {
            aux.add(snapshot.aux[i]);
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // System Settings API
    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request){
        bool armed = false;
        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            armed = tankController.isSystemArmed();
            xSemaphoreGive(tankMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
            return;
        }

        JsonDocument doc;
        doc["debug"] = Config::DEBUG_ENABLED;
        doc["armed"] = armed;
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        JsonDocument doc;
        deserializeJson(doc, data);

        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (!doc["debug"].isNull()) {
                Config::DEBUG_ENABLED = doc["debug"];
                tankController.setDebugMode(Config::DEBUG_ENABLED);
            }
            if (!doc["armed"].isNull()) {
                tankController.setSystemArmed(doc["armed"]);
            }
            xSemaphoreGive(tankMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
            return;
        }

        request->send(200, "application/json", "{\"status\":\"success\"}");
    });

    // Not found
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });
}
