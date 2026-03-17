#include "web_server_manager.h"
#include "web_pages.h"
#include "controllers/tank_controller.h"
#include "config/config.h"
#include <ArduinoJson.h>
#include "freertos/semphr.h"

#include "controllers/tank_controller.h"

extern TankController tankController;
extern PendingConfig pendingConfig;
extern volatile bool pendingReboot;

// Mutex definido em main.cpp — protege o tankController contra acesso simultâneo
// entre Core 0 (web callbacks) e Core 1 (TankControlTask).
extern SemaphoreHandle_t tankMutex;

// Cache de dados estáticos do hardware — preenchido em begin() para evitar acessos SPI no hot path
static String hw_chip_model;
static int    hw_chip_revision;
static int    hw_cpu_freq;
static uint32_t hw_heap_total;
static uint32_t hw_flash_size;
static uint32_t hw_sketch_size;
static String hw_mac;

WebServerManager::WebServerManager(const char* ssid, const char* password)
    : ap_ssid(ssid), ap_password(password), server(80) {}

bool WebServerManager::begin() {
    if (Config::WIFI_MODE == 1 && Config::STA_SSID.length() > 0) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Tentando conectar à rede Wi-Fi (Modo Station)... SSID: %s", Config::STA_SSID.c_str());
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(Config::STA_SSID.c_str(), Config::STA_PASS.c_str());
        
        // Aguarda conexão por até 10 segundos
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Conectado com sucesso! IP do Rover: %s", WiFi.localIP().toString().c_str());
        } else {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_WARN, "Falha ao conectar. Retornando para Modo AP de segurança.");
            Config::WIFI_MODE = 0; // Fallback temporário em memória (não salva no NVS para não sobrescrever a config do usuário)
        }
    }

    // Se estiver configurado para AP ou se o Station falhou no fallback
    if (Config::WIFI_MODE == 0) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Iniciando modo Access Point...");
        WiFi.mode(WIFI_AP);
        if (!WiFi.softAP(ap_ssid, ap_password)) {
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao iniciar Access Point!");
            return false;
        }

        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "AP iniciado: %s | IP: %s", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
    }

    // Pré-computa os dados estáticos sem risco de suspensão de scheduler no hot path
    hw_chip_model    = ESP.getChipModel();
    hw_chip_revision = ESP.getChipRevision();
    hw_cpu_freq      = ESP.getCpuFreqMHz();
    hw_heap_total    = ESP.getHeapSize();
    hw_flash_size    = ESP.getFlashChipSize();
    hw_sketch_size   = ESP.getSketchSize();
    hw_mac           = WiFi.macAddress();

    setupRoutes();

    server.begin();
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Servidor Async iniciado na porta 80");

    return true;
}

void WebServerManager::setupRoutes() {
    // Servir arquivos embarcados na memória (PROGMEM) para evitar conflitos com SPIFFS/Interrupts
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/css", style_css);
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/javascript", script_js);
    });

    // Health check
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
    });

    // System Info API — usa apenas valores em cache (sem acessos SPI no hot path)
    server.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["chip_model"]    = hw_chip_model;
        doc["chip_revision"] = hw_chip_revision;
        doc["cpu_freq"]      = hw_cpu_freq;
        doc["heap_total"]    = hw_heap_total;
        doc["flash_size"]    = hw_flash_size;
        doc["sketch_size"]   = hw_sketch_size;
        doc["uptime"]        = millis();
        doc["mac"]           = hw_mac;
        
        if (Config::WIFI_MODE == 1 && WiFi.status() == WL_CONNECTED) {
            doc["ip"]      = WiFi.localIP().toString();
            doc["ssid"]    = WiFi.SSID();
            doc["mode"]    = "Station";
            doc["gateway"] = WiFi.gatewayIP().toString();
            doc["channel"] = WiFi.channel();
        } else {
            doc["ip"]      = WiFi.softAPIP().toString();
            doc["ssid"]    = WiFi.softAPSSID();
            doc["mode"]    = "AP";
            doc["clients"] = WiFi.softAPgetStationNum();
            doc["channel"] = WiFi.channel();
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // RC Channels API
    server.on("/api/channels", HTTP_GET, [](AsyncWebServerRequest *request){
        // Copia os dados para variáveis locais dentro da região crítica,
        // evitando uso de referência após liberar o mutex.
        Types::ChannelData snapshot;
        if (tankMutex == nullptr) {
            request->send(503, "application/json", "{\"error\":\"system not ready\"}");
            return;
        }
        if (xSemaphoreTake(tankMutex, 0) == pdTRUE) {
            snapshot = tankController.getChannelData();
            xSemaphoreGive(tankMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
            return;
        }

        JsonDocument doc;
        doc["valid"] = snapshot.isValid;
        
        // Envia todos os canais brutos em sequência CH1 a CH10 (assumindo Steering no CH1 e Throttle no CH3 para carros)
        JsonArray raw = doc["raw_channels"].to<JsonArray>();
        // O ChannelManager guarda steering e throttle em variáveis e o resto em aux, 
        // mas idealmente ele deveria guardar tudo num array de 10 posições,
        // como a estrutura Types::ChannelData atual tem steering, throttle e aux[8], 
        // Vamos reconstruir os 10 canais lógicos:
        raw.add(snapshot.steering); // CH1
        raw.add(snapshot.aux[0]);   // CH2
        raw.add(snapshot.throttle); // CH3
        raw.add(snapshot.aux[1]);   // CH4
        raw.add(snapshot.aux[2]);   // CH5
        raw.add(snapshot.aux[3]);   // CH6
        raw.add(snapshot.aux[4]);   // CH7
        raw.add(snapshot.aux[5]);   // CH8
        raw.add(snapshot.aux[6]);   // CH9
        raw.add(snapshot.aux[7]);   // CH10

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Logs API - Serve o buffer circular de logs em texto puro
    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){
        String logs = tankController.getSystemLogs();
        request->send(200, "text/plain", logs);
    });

    // Limpar logs
    server.on("/api/clear-logs", HTTP_POST, [](AsyncWebServerRequest *request){
        tankController.clearSystemLogs();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // IMU / Sensors API
    server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
        Types::ImuData imuSnapshot;
        Types::GpsData gpsSnapshot;
        Types::CompassData compassSnapshot;
        Types::MotorCommands motorSnapshot;
        bool isArmedSnapshot = false;
        
        if (tankMutex == nullptr) {
            request->send(503, "application/json", "{\"error\":\"system not ready\"}");
            return;
        }
        if (xSemaphoreTake(tankMutex, 0) == pdTRUE) {
            imuSnapshot = tankController.getImuData();
            gpsSnapshot = tankController.getGpsData();
            compassSnapshot = tankController.getCompassData();
            motorSnapshot = tankController.getMotorCommands();
            isArmedSnapshot = tankController.isSystemArmed();
            xSemaphoreGive(tankMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
            return;
        }

        JsonDocument doc;
        
        // IMU Data
        doc["valid"]       = imuSnapshot.isValid; // kept for backwards compatibility (frontend)
        doc["temperature"] = imuSnapshot.temperature;

        JsonObject accel = doc["accel"].to<JsonObject>();
        accel["x"] = imuSnapshot.accelX;
        accel["y"] = imuSnapshot.accelY;
        accel["z"] = imuSnapshot.accelZ;

        JsonObject gyro = doc["gyro"].to<JsonObject>();
        gyro["x"] = imuSnapshot.gyroX;
        gyro["y"] = imuSnapshot.gyroY;
        gyro["z"] = imuSnapshot.gyroZ;

        JsonObject angles = doc["angles"].to<JsonObject>();
        angles["roll"]  = imuSnapshot.roll;
        angles["pitch"] = imuSnapshot.pitch;
        angles["yaw"]   = imuSnapshot.yaw;

        // GPS Data
        JsonObject gps = doc["gps"].to<JsonObject>();
        gps["valid"] = gpsSnapshot.isValid;
        if(gpsSnapshot.isValid) {
            gps["lat"] = gpsSnapshot.latitude;
            gps["lng"] = gpsSnapshot.longitude;
            gps["alt"] = gpsSnapshot.altitude;
            gps["speed"] = gpsSnapshot.speed;
            gps["course"] = gpsSnapshot.course;
            gps["satellites"] = gpsSnapshot.satellites;
            gps["hdop"] = gpsSnapshot.hdop;
            gps["time"] = gpsSnapshot.dateTime;
        }

        // Compass Data
        JsonObject compass = doc["compass"].to<JsonObject>();
        compass["valid"] = compassSnapshot.isValid;
        compass["heading"] = compassSnapshot.heading;
        compass["x"] = compassSnapshot.x;
        compass["y"] = compassSnapshot.y;
        compass["z"] = compassSnapshot.z;

        // System & Motor Data
        doc["armed"] = isArmedSnapshot;
        JsonObject motors = doc["motors"].to<JsonObject>();
        motors["left"] = motorSnapshot.left;
        motors["right"] = motorSnapshot.right;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // System Settings API
    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request){
        if (tankMutex == nullptr) {
            request->send(503, "application/json", "{\"error\":\"system not ready\"}");
            return;
        }

        // isSystemArmed é atomic volatile agora. Seguro ler sem mutex.
        bool armed = tankController.isSystemArmed();

        JsonDocument doc;
        doc["dark_theme"] = Config::DARK_THEME;
        doc["debug"] = Config::DEBUG_ENABLED;
        doc["armed"] = armed;
        doc["wifi_mode"] = Config::WIFI_MODE;
        doc["sta_ssid"] = Config::STA_SSID;
        // Não enviamos a senha por segurança
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Calibrate IMU API
    server.on("/api/calibrate-imu", HTTP_POST, [](AsyncWebServerRequest *request){
        if (tankMutex == nullptr) {
            request->send(503, "application/json", "{\"error\":\"system not ready\"}");
            return;
        }
        
        // A calibração manipula flags simples de estado da IMU. Pode ser feito de forma tolerante,
        // mas vamos manter o Try-Lock com 0 Ticks pra ser Async Safe. 
        if (xSemaphoreTake(tankMutex, 0) == pdTRUE) {
            tankController.calibrateImu();
            xSemaphoreGive(tankMutex);
            request->send(200, "application/json", "{\"status\":\"started\"}");
        } else {
            request->send(503, "application/json", "{\"error\":\"system busy\"}");
        }
    });

    server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        JsonDocument doc;
        deserializeJson(doc, data);

        bool requiresReboot = false;

        // Captura os novos estados para processamento seguro na Main Task
        // Não mexemos no NVS nem no TankController aqui para evitar Kernel Panic.
        
        pendingConfig.debugEnabled = !doc["debug"].isNull() ? (bool)doc["debug"] : Config::DEBUG_ENABLED;
        pendingConfig.darkTheme    = !doc["dark_theme"].isNull() ? (bool)doc["dark_theme"] : Config::DARK_THEME;
        
        bool armedSnapshot;
        if (tankMutex != nullptr && xSemaphoreTake(tankMutex, 0) == pdTRUE) {
            armedSnapshot = tankController.isSystemArmed();
            xSemaphoreGive(tankMutex);
        } else {
            armedSnapshot = false; // Fallback se ocupado, idealmente mantemos o estado
        }
        pendingConfig.systemArmed   = !doc["armed"].isNull() ? (bool)doc["armed"] : armedSnapshot;

        if (!doc["wifi_mode"].isNull()) {
            pendingConfig.wifiChange = true;
            pendingConfig.wifiMode   = doc["wifi_mode"];
            pendingConfig.wifiSSID   = doc["sta_ssid"] | Config::STA_SSID;
            pendingConfig.wifiPass   = doc["sta_pass"] | Config::STA_PASS;
            requiresReboot = true;
        }

        pendingConfig.hasChanges = true;

        if (requiresReboot) {
            request->send(200, "application/json", "{\"status\":\"rebooting\"}");
            // Sinaliza para a main task efetuar o restart (onde delay é seguro)
            pendingReboot = true;
        } else {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        }
    });

    // Not found
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });
}
