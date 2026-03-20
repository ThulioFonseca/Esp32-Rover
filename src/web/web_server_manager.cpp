#include "web_server_manager.h"
#include "web_pages.h"
#include "controllers/tank_controller.h"
#include "config/config.h"
#include <ArduinoJson.h>
#include "freertos/semphr.h"

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
    : ap_ssid(ssid), ap_password(password), server(80), ws("/ws") {}

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

    setupWebSocket();
    setupRoutes();

    server.begin();
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Servidor Async iniciado na porta 80 (WebSocket em /ws)");

    return true;
}

void WebServerManager::setupRoutes() {
    // Assets estáticos servidos com GZIP + Cache agressivo (Fase 1).
    // Content-Encoding: gzip — o browser descomprime, poupando banda Wi-Fi.
    // Cache-Control: max-age=31536000 — browser só busca no primeiro acesso.

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
        r->redirect("/index.html");
    });

    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse(200, "text/html", index_html_gz, index_html_gz_len);
        r->addHeader("Content-Encoding", "gzip");
        r->addHeader("Cache-Control", "max-age=31536000, public");
        request->send(r);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse(200, "text/css", style_css_gz, style_css_gz_len);
        r->addHeader("Content-Encoding", "gzip");
        r->addHeader("Cache-Control", "max-age=31536000, public");
        request->send(r);
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse(200, "application/javascript", script_js_gz, script_js_gz_len);
        r->addHeader("Content-Encoding", "gzip");
        r->addHeader("Cache-Control", "max-age=31536000, public");
        request->send(r);
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

// ─── WebSocket ───────────────────────────────────────────────────────────────

void WebServerManager::setupWebSocket() {
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onWsEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "WebSocket handler registrado em /ws");
}

void WebServerManager::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO,
                "WS cliente #%u conectado de %s", client->id(), client->remoteIP().toString().c_str());
            break;

        case WS_EVT_DISCONNECT:
            tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO,
                "WS cliente #%u desconectado", client->id());
            break;

        case WS_EVT_DATA: {
            // Mensagens do cliente → comandos leves (ex: calibrate)
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0; // null-terminate
                String msg = (char*)data;


            }
            break;
        }

        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

size_t WebServerManager::wsClientCount() const {
    return ws.count();
}

/**
 * broadcastSensorData() — chamado pela wsBroadcastTask (Core 0) a cada 50 ms (20 Hz).
 *
 * Otimizações para evitar memory corruption (Guru Meditation Error):
 *   - Usa StaticJsonDocument (alocado na stack, não heap)
 *   - Buffer pré-alocado para serialização
 *   - Sem String temporária que causa move semantics perigosa
 */

// ── Fase 3: Buffer binário estático (121 bytes, little-endian) ───────────────
// Protocolo: packet_type(1) | IMU(37) | GPS(32) | Compass(17) | Motors(8) | CH(21) | Sys(5)
// Elimina serialização JSON (economiza ~230 bytes/frame × 20Hz = ~4.5 KB/s de banda)
static uint8_t wsBinaryBuffer[Config::WS_BINARY_FRAME_SIZE];

void WebServerManager::broadcastSensorData() {
    ws.cleanupClients(2);
    if (ws.count() == 0) return;
    if (tankMutex == nullptr) return;

    // ── 1. Snapshot atômico ──
    Types::ImuData       imu;
    Types::GpsData       gps;
    Types::CompassData   compass;
    Types::MotorCommands motors;
    Types::ChannelData   channels;
    bool                 armed = false;

    if (xSemaphoreTake(tankMutex, 0) != pdTRUE) {
        return;
    }

    imu      = tankController.getImuData();
    gps      = tankController.getGpsData();
    compass  = tankController.getCompassData();
    motors   = tankController.getMotorCommands();
    channels = tankController.getChannelData();
    armed    = tankController.isSystemArmed();

    xSemaphoreGive(tankMutex);

    // ── 2. Packing binário com memcpy (evita UB de strict-aliasing) ──
    uint8_t* p = wsBinaryBuffer;

    auto writeU8  = [&](uint8_t  v) { *p++ = v; };
    auto writeU32 = [&](uint32_t v) { memcpy(p, &v, 4); p += 4; };
    auto writeI16 = [&](int16_t  v) { memcpy(p, &v, 2); p += 2; };
    auto writeF32 = [&](float    v) { memcpy(p, &v, 4); p += 4; };

    writeU8(0x01); // packet_type = sensor frame

    // IMU (37 bytes: offset 1–37) — yaw removido (MPU-6500 sem magnetômetro)
    writeF32(imu.roll);   writeF32(imu.pitch);
    writeF32(imu.accelX); writeF32(imu.accelY); writeF32(imu.accelZ);
    writeF32(imu.gyroX);  writeF32(imu.gyroY);  writeF32(imu.gyroZ);
    writeF32(imu.temperature);
    writeU8(imu.isValid ? 1 : 0);

    // GPS (32 bytes: offset 38–69)
    writeF32(gps.latitude);  writeF32(gps.longitude); writeF32(gps.altitude);
    writeF32(gps.speed);     writeF32(gps.course);
    writeU32(gps.satellites); writeF32(gps.hdop);
    writeU8(gps.timeHour); writeU8(gps.timeMinute); writeU8(gps.timeSecond);
    writeU8(gps.isValid ? 1 : 0);

    // Compass (17 bytes: offset 70–86)
    writeF32(compass.heading);
    writeF32(compass.x); writeF32(compass.y); writeF32(compass.z);
    writeU8(compass.isValid ? 1 : 0);

    // Motors (8 bytes: offset 87–94)
    writeF32(motors.left); writeF32(motors.right);

    // RC Channels — 10 × int16 + valid (21 bytes: offset 95–115)
    writeI16((int16_t)channels.steering);
    writeI16((int16_t)channels.aux[0]);
    writeI16((int16_t)channels.throttle);
    writeI16((int16_t)channels.aux[1]);
    writeI16((int16_t)channels.aux[2]);
    writeI16((int16_t)channels.aux[3]);
    writeI16((int16_t)channels.aux[4]);
    writeI16((int16_t)channels.aux[5]);
    writeI16((int16_t)channels.aux[6]);
    writeI16((int16_t)channels.aux[7]);
    writeU8(channels.isValid ? 1 : 0);

    // System (5 bytes: offset 116–120)
    writeU8(armed ? 1 : 0);
    writeU32(millis());

    // ── 3. Envia como frame binário WebSocket ──
    // p - wsBinaryBuffer deve ser exatamente 121 bytes
    ws.binaryAll(wsBinaryBuffer, (size_t)(p - wsBinaryBuffer));
}
