/**
 * ESP32 Rover - Main Entry Point
 *
 * Arquitetura de tasks (FreeRTOS dual-core):
 *   Core 0 — Wi-Fi / Web Server (AsyncWebServer + LwIP) + loopTask (Arduino)
 *   Core 1 — Controle em tempo real (iBUS + ESCs, 50 Hz fixo)
 *
 * A inicialização é sequencial: operações de Flash (Wi-Fi/NVS) são concluídas
 * antes de habilitar as interrupções de hardware (Servos/IBUS), evitando
 * o crash documentado nos SDKs Espressif quando ambos ocorrem simultaneamente.
 *
 * Flash Write Safety:
 *   Gravações NVS (savePreferences) ocorrem apenas no setup() do boot seguinte,
 *   via RTC_NOINIT_ATTR.
 */

#include "controllers/tank_controller.h"
#include "web/web_server_manager.h"
#include "config/config.h"
#include "freertos/semphr.h"
#include "config/pins.h"
#include "utils/status_led_manager.h"
#include <cstring>

// Struct em memória RTC (sobrevive soft reset via ESP.restart(), mas não power cycle).
// Permite passar configurações pendentes para o próximo boot, onde o NVS será gravado
// com segurança total (nenhuma task FreeRTOS rodando além do loopTask).
RTC_NOINIT_ATTR struct {
    uint32_t magic;
    bool     debugEnabled;
    bool     darkTheme;
    uint8_t  wifiMode;
    char     ssid[33];
    char     pass[65];
} pendingNvsRtc;

static const uint32_t RTC_NVS_MAGIC = 0xC0FFEE42;

TankController tankController;
WebServerManager webServer("ESP32-ROVER", "rover1234");
StatusLedManager statusLed(LED_BUILTIN);

// Protege tankController contra acesso concorrente entre Core 0 (web) e Core 1 (controle).
SemaphoreHandle_t tankMutex = NULL;
PendingConfig pendingConfig;
volatile bool pendingReboot  = false;
volatile bool pendingNvsSave = false;  // true = há configurações não persistidas no NVS

// Executa o loop de controle a 50 Hz usando vTaskDelayUntil para periodicidade determinística.
void tankControlTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::CONTROL_INTERVAL_MS);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        statusLed.update();

        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(Config::TANK_MUTEX_TIMEOUT_MS)) == pdTRUE) {
            tankController.update();
            xSemaphoreGive(tankMutex);
        }
    }
}

// Atualiza sensores I2C/UART a 50 Hz no Core 1, sem segurar o tankMutex.
// Prioridade 1 (< tankControlTask) — preemptada pelo loop de controle quando necessário.
// O driver I2C do ESP-IDF é baseado em DMA/interrupção: continua operando durante preempções.
void sensorUpdateTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::CONTROL_INTERVAL_MS);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        tankController.updateSensors();
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

    // Aplica config pendente do boot anterior (salva no NVS com zero tasks rodando).
    // Neste ponto do setup() só existe o loopTask e os IDLE tasks — sem WiFi,
    // AsyncTCP, ou qualquer semáforo ativo. É o momento mais seguro para flash write.
    if (pendingNvsRtc.magic == RTC_NVS_MAGIC) {
        pendingNvsRtc.magic = 0;  // consome — não repete no próximo boot
        Config::DEBUG_ENABLED = pendingNvsRtc.debugEnabled;
        Config::DARK_THEME    = pendingNvsRtc.darkTheme;
        Config::WIFI_MODE     = pendingNvsRtc.wifiMode;
        Config::STA_SSID      = String(pendingNvsRtc.ssid);
        Config::STA_PASS      = String(pendingNvsRtc.pass);
        Config::savePreferences();
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Config pendente aplicada e salva no NVS");
    }

    // Carregar configurações persistentes (WIFI, etc)
    Config::loadPreferences();
    tankController.debugManager.setEnabled(Config::DEBUG_ENABLED);

    // 1. Criar primitivas de sincronização antes de iniciar o web server.
    tankMutex = xSemaphoreCreateMutex();
    if (tankMutex == NULL) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao criar tankMutex — sistema travado");
        statusLed.setStatus(LED_STATUS_ERROR);
        while (true) {
            statusLed.update();
            delay(10);
        }
    }

    // 2. Wi-Fi e Web Server — pode escrever em Flash/NVS; deve ocorrer antes das interrupts.
    if (!webServer.begin()) {
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_ERROR, "Falha ao iniciar Web Server");
        statusLed.setStatus(LED_STATUS_ERROR);
        while (true) { 
            statusLed.update();
            delay(10); 
        }
    }

    // 3. TankController — habilita interrupts de hardware (Servos + Serial iBUS).
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

    // 4. Tasks do sistema.
    // Core 0: wsBroadcastTask (20 Hz) + tasks internas do AsyncWebServer/AsyncTCP
    // Core 1: tankControlTask (50 Hz, prioridade alta)
    xTaskCreatePinnedToCore(wsBroadcastTask,  "WsBroadcastTask",  4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(tankControlTask,  "TankControlTask",  8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(sensorUpdateTask, "SensorUpdateTask", 4096, NULL, 1, NULL, 1);

    tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Tasks iniciadas — controle 50 Hz | sensores 50 Hz | WS broadcast 20 Hz");
}

void loop() {
    // Reboot tem prioridade máxima.
    // Aplica config pendente, salva NVS com tasks Core 1 estacionadas, depois reinicia.
    if (pendingReboot) {
        pendingReboot = false;

        // 1. Aplica config pendente na RAM
        if (pendingConfig.hasChanges) {
            pendingConfig.hasChanges = false;
            Config::DEBUG_ENABLED = pendingConfig.debugEnabled;
            Config::DARK_THEME    = pendingConfig.darkTheme;
            tankController.setDebugMode(pendingConfig.debugEnabled);
            tankController.setSystemArmed(pendingConfig.systemArmed);
            if (pendingConfig.wifiChange) {
                pendingConfig.wifiChange = false;
                Config::WIFI_MODE = pendingConfig.wifiMode;
                Config::STA_SSID  = pendingConfig.wifiSSID;
                Config::STA_PASS  = pendingConfig.wifiPass;
            }
            pendingNvsSave = true;
        }

        // 2. Copia config para RTC memory (sobrevive soft reset).
        //    NVS será gravado no setup() do próximo boot, sem nenhuma task rodando.
        if (pendingNvsSave) {
            pendingNvsSave = false;
            pendingNvsRtc.debugEnabled = Config::DEBUG_ENABLED;
            pendingNvsRtc.darkTheme    = Config::DARK_THEME;
            pendingNvsRtc.wifiMode     = Config::WIFI_MODE;
            strncpy(pendingNvsRtc.ssid, Config::STA_SSID.c_str(), sizeof(pendingNvsRtc.ssid) - 1);
            pendingNvsRtc.ssid[sizeof(pendingNvsRtc.ssid) - 1] = '\0';
            strncpy(pendingNvsRtc.pass, Config::STA_PASS.c_str(), sizeof(pendingNvsRtc.pass) - 1);
            pendingNvsRtc.pass[sizeof(pendingNvsRtc.pass) - 1] = '\0';
            pendingNvsRtc.magic = RTC_NVS_MAGIC;
        }

        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Reiniciando o sistema...");
        delay(300);
        ESP.restart();
    }

    // Aplica mudanças de config na RAM imediatamente.
    // NVS só é gravado no próximo reboot (evita crash por flash write em runtime).
    if (pendingConfig.hasChanges) {
        pendingConfig.hasChanges = false;

        Config::DEBUG_ENABLED = pendingConfig.debugEnabled;
        Config::DARK_THEME    = pendingConfig.darkTheme;
        tankController.setDebugMode(pendingConfig.debugEnabled);
        tankController.setSystemArmed(pendingConfig.systemArmed);

        if (pendingConfig.wifiChange) {
            pendingConfig.wifiChange = false;
            Config::WIFI_MODE = pendingConfig.wifiMode;
            Config::STA_SSID  = pendingConfig.wifiSSID;
            Config::STA_PASS  = pendingConfig.wifiPass;
        }

        pendingNvsSave = true;
        tankController.debugManager.logf(DebugManager::LOG_LEVEL_INFO, "Configurações aplicadas (NVS será gravado no próximo reboot).");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
