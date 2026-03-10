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

TankController tankController;
WebServerManager webServer("ESP32-ROVER", "rover1234");

// Protege tankController contra acesso concorrente entre Core 0 (web) e Core 1 (controle).
SemaphoreHandle_t tankMutex = NULL;

// Executa o loop de controle a 50 Hz usando vTaskDelayUntil para periodicidade determinística.
void tankControlTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::CONTROL_INTERVAL_MS);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Timeout de 5 ms: se a web retiver o mutex, este ciclo é descartado sem travar o sistema.
        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            tankController.update();
            xSemaphoreGive(tankMutex);
        }
    }
}

// Task mantida no Core 0 para preservar o contexto do scheduler do lado da rede.
void webServerTask(void* pvParameters) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Iniciando Sistema ===");

    // Carregar configurações persistentes (WIFI, etc)
    Config::loadPreferences();

    // 1. SPIFFS — sem interrupções de hardware ativas.
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERRO] Falha ao montar SPIFFS");
    }

    // 2. Wi-Fi e Web Server — pode escrever em Flash/NVS; deve ocorrer antes das interrupts.
    if (!webServer.begin()) {
        Serial.println("[ERRO] Falha ao iniciar Web Server");
    }

    // 3. TankController — habilita interrupts de hardware (Servos + Serial iBUS).
    if (!tankController.initialize()) {
        Serial.println("[ERRO] Falha crítica no TankController — sistema travado");
        while (true) { delay(1000); }
    }

    // 4. Mutex criado após inicialização e antes da criação das tasks.
    tankMutex = xSemaphoreCreateMutex();
    if (tankMutex == NULL) {
        Serial.println("[ERRO] Falha ao criar mutex — sistema travado");
        while (true) { delay(1000); }
    }

    // 5. Tasks do sistema.
    xTaskCreatePinnedToCore(webServerTask,   "WebServerTask",  8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(tankControlTask, "TankControlTask", 8192, NULL, 2, NULL, 1);

    Serial.println("[INFO] Tasks iniciadas — sistema operacional");
}

void loop() {
    // Controle roda na TankControlTask; loop liberado para o scheduler do Core 1.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
