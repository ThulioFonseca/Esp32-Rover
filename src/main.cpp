/**
 * ESP32 Rover - Main Entry Point
 *
 * Arquitetura de tasks (FreeRTOS dual-core):
 *   Core 0 — Wi-Fi / Web Server (AsyncWebServer + LwIP)
 *   Core 1 — Controle em tempo real (iBUS + ESCs, 50 Hz fixo)
 *
 * A inicialização é sequencial: operações de Flash (Wi-Fi/NVS) são concluídas
 * antes de habilitar as interrupções de hardware (Servos/IBUS), evitando
 * o crash documentado nos SDKs Espressif quando ambos ocorrem simultaneamente.
 */

#include "controllers/tank_controller.h"
#include "web/web_server_manager.h"
#include "config/config.h"
#include "freertos/semphr.h"
#include <SPIFFS.h>
#include "config/pins.h"
#include "utils/status_led_manager.h"

TankController tankController;
WebServerManager webServer("ESP32-ROVER", "rover1234");
StatusLedManager statusLed(LED_BUILTIN);

// Protege tankController contra acesso concorrente entre Core 0 (web) e Core 1 (controle).
SemaphoreHandle_t tankMutex = NULL;
PendingConfig pendingConfig;
volatile bool pendingReboot = false;

// Executa o loop de controle a 50 Hz usando vTaskDelayUntil para periodicidade determinística.
void tankControlTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::CONTROL_INTERVAL_MS);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Atualiza a sinalização visual (não obstrutiva, demora nanosegundos)
        statusLed.update();

        // Se a web retiver o mutex, este ciclo é descartado sem travar o sistema.
        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(Config::TANK_MUTEX_TIMEOUT_MS)) == pdTRUE) {
            tankController.update();
            xSemaphoreGive(tankMutex);
        }
    }
}

// Broadcast de dados via WebSocket a 20 Hz (50 ms) no Core 0.
// Usa vTaskDelayUntil para periodicidade determinística, sem drift.
// broadcastSensorData() faz try-lock no mutex (0 ticks) — nunca bloqueia o Core 1.
void wsBroadcastTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::WS_BROADCAST_INTERVAL_MS);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        webServer.broadcastSensorData();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializa o gerenciador de logs cedo para usar nas falhas iniciais se possível
    tankController.debugManager.initialize();
    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "=== Iniciando Sistema ===");

    statusLed.begin();
    // Inicia marcando como Warning enquanto não inicializa os periféricos vitais
    statusLed.setStatus(LED_STATUS_WARNING);

    // Carregar configurações persistentes (WIFI, etc)
    Config::loadPreferences();
    tankController.debugManager.setEnabled(Config::DEBUG_ENABLED);

    // 1. SPIFFS — sem interrupções de hardware ativas.
    if (!SPIFFS.begin(true)) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao montar SPIFFS");
    }

    // 2. Criar Mutex antes de iniciar o web server (evita crash se requests chegarem cedo).
    tankMutex = xSemaphoreCreateMutex();
    if (tankMutex == NULL) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao criar mutex — sistema travado");
        statusLed.setStatus(LED_STATUS_ERROR);
        while (true) { 
            statusLed.update();
            delay(10); 
        }
    }

    // 3. Wi-Fi e Web Server — pode escrever em Flash/NVS; deve ocorrer antes das interrupts.
    if (!webServer.begin()) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao iniciar Web Server");
        statusLed.setStatus(LED_STATUS_ERROR);
        while (true) { 
            statusLed.update();
            delay(10); 
        }
    }

    // 4. TankController — habilita interrupts de hardware (Servos + Serial iBUS).
    if (!tankController.initialize()) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha crítica no TankController — sistema travado");
        statusLed.setStatus(LED_STATUS_ERROR);
        while (true) { 
            statusLed.update();
            delay(10); 
        }
    }

    // Se no WebServerManager caiu em fallback pro AP Mode, mantém como Warning
    if (Config::WIFI_MODE == 0) {
        statusLed.setStatus(LED_STATUS_WARNING);
    } else {
        statusLed.setStatus(LED_STATUS_OPERATIONAL);
    }

    // 5. Tasks do sistema.
    // Core 0: wsBroadcastTask (20 Hz) + tasks internas do AsyncWebServer/AsyncTCP
    // Core 1: tankControlTask (50 Hz, prioridade alta)
    xTaskCreatePinnedToCore(wsBroadcastTask,  "WsBroadcastTask", 4096, NULL, 1, NULL, 0);  // Broadcast binário compacto (122 bytes) — stack 4 KB é suficiente
    xTaskCreatePinnedToCore(tankControlTask,  "TankControlTask", 8192, NULL, 2, NULL, 1);

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Tasks iniciadas — sistema operacional (WS broadcast 20 Hz)");
}

void loop() {
    // Se a API web sinalizou alguma mudança de config, aplicamos e salvamos aqui 
    // no Main Loop (Core 1 / Seguro), fora da Task de Rede do ESP32 para não dar Assert
    if (pendingConfig.hasChanges) {
        pendingConfig.hasChanges = false;
        
        // 1. Aplica no controlador
        tankController.setDebugMode(pendingConfig.debugEnabled);
        tankController.setSystemArmed(pendingConfig.systemArmed);
        
        // 2. Atualiza os globais do Config
        Config::DEBUG_ENABLED = pendingConfig.debugEnabled;
        Config::DARK_THEME = pendingConfig.darkTheme;
        
        // 3. Persiste no NVS (Operação pesada/bloqueante - agora segura aqui)
        Config::saveThemePreference(Config::DARK_THEME);
        Config::saveDebugPreference(Config::DEBUG_ENABLED);
        
        if (pendingConfig.wifiChange) {
            pendingConfig.wifiChange = false;
            Config::WIFI_MODE = pendingConfig.wifiMode;
            Config::STA_SSID = pendingConfig.wifiSSID;
            Config::STA_PASS = pendingConfig.wifiPass;
            Config::saveNetworkPreferences(Config::WIFI_MODE, Config::STA_SSID, Config::STA_PASS);
        }
        
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Configurações aplicadas e persistidas no NVS com sucesso.");
    }

    if (pendingReboot) {
        pendingReboot = false;
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Reiniciando o sistema...");
        delay(500);
        ESP.restart();
    }

    // Controle em tempo real roda na TankControlTask; loop liberado
    vTaskDelay(pdMS_TO_TICKS(1000));
}
