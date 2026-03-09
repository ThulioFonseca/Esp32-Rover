/**
 * ESP32 Rover - Main Entry Point
 * 
 * Arquitetura do Sistema:
 * - O sistema utiliza FreeRTOS para gerenciar tarefas em ambos os núcleos do ESP32.
 * - Core 0: Dedicado a tarefas de sistema e rede (WiFi/Web Server).
 * - Core 1: Dedicado ao controle em tempo real do rover (Motores/Sensores/RC).
 * 
 * Estratégia de Inicialização:
 * A inicialização é sequencial para evitar conflitos de hardware, especificamente
 * entre operações de escrita na Flash (WiFi/NVS) e interrupções de hardware (Servos/IBUS).
 */

#include "controllers/tank_controller.h"
#include "web/web_server_manager.h"
#include "config/config.h"
#include "freertos/semphr.h"

// Instâncias globais dos subsistemas
TankController tankController;
WebServerManager webServer("ESP32-ROVER", "rover1234");

// Mutex para proteger o acesso ao tankController entre os dois núcleos:
// - Core 1: TankControlTask (escrita a 50Hz)
// - Core 0: Callbacks do AsyncWebServer (leitura/escrita esporádica)
SemaphoreHandle_t tankMutex = NULL;

/**
 * Task de Controle em Tempo Real (Core 1)
 * Executa o loop de controle PID e processamento de sinais RC.
 * Utiliza vTaskDelayUntil para garantir frequência fixa e determinística.
 */
void tankControlTask(void* pvParameters) {
    Serial.println("[INFO] Iniciando Task de Controle (Core 1)");
    
    TickType_t xLastWakeTime;
    // Frequência fixa de execução (50Hz / 20ms)
    const TickType_t xFrequency = pdMS_TO_TICKS(Config::CONTROL_INTERVAL_MS);

    // Inicializa o tempo de referência
    xLastWakeTime = xTaskGetTickCount();

    while (true) {
        // Bloqueia a task até o próximo ciclo de 20ms
        // Garante precisão temporal e libera CPU para tarefas de menor prioridade (IDLE/Watchdog)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Adquire o mutex com timeout curto (5ms) para não comprometer o ciclo de 20ms.
        // Se o Core 0 retiver o mutex por mais tempo, este ciclo é pulado com segurança.
        if (xSemaphoreTake(tankMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            tankController.update();
            xSemaphoreGive(tankMutex);
        }
    }
}

/**
 * Task de Suporte ao Web Server (Core 0)
 * O servidor Async roda nas tasks do sistema (LwIP), mas esta task
 * mantém o contexto vivo e pode ser usada para monitoramento futuro.
 */
void webServerTask(void* pvParameters) {
    Serial.println("[INFO] WebServer rodando na task dedicada (Core 0)");
    
    while (true) {
        // Mantém a task suspensa a maior parte do tempo para não consumir CPU
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void setup() {
  // Inicializa Serial para debug antes de qualquer coisa
  Serial.begin(115200);
  delay(1000); // Aguarda serial estabilizar
  Serial.println("\n=== Iniciando Sistema ===");

  // 1. Inicializa SPIFFS (Seguro, sem interrupções)
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar SPIFFS");
  } else {
    Serial.println("SPIFFS montado com sucesso");
  }

  // 2. Inicializa Web Server e WiFi (Pode escrever na Flash/NVS)
  // Fazemos isso ANTES de ativar interrupções de hardware para evitar conflitos
  if (!webServer.begin()) {
    Serial.println("[ERRO] Falha ao iniciar servidor!");
  } else {
    Serial.println("[INFO] Web Server iniciado com sucesso");
  }

  // 3. Inicializa TankController (Ativa Interrupções de Hardware: Servos, Serial/IBUS)
  // Agora é seguro pois Flash operations já ocorreram
  if (!tankController.initialize()) {
    Serial.println("ERRO: Falha na inicialização do TankController!");
    while(1) {
      delay(1000);
    }
  }

    // 4. Cria o mutex de sincronização entre cores ANTES das tasks
    tankMutex = xSemaphoreCreateMutex();
    if (tankMutex == NULL) {
        Serial.println("[ERRO] Falha ao criar mutex! Sistema não pode iniciar.");
        while(1) { delay(1000); }
    }
    Serial.println("[INFO] Mutex de sincronização criado");

    // 5. Cria as tasks do sistema

    // Cria a task do WebServer no Core 0
    xTaskCreatePinnedToCore(
        webServerTask,       // Função da task
        "WebServerTask",     // Nome da task
        8192,                // Stack em bytes
        NULL,                // Parâmetros da task
        1,                   // Prioridade 1 (Normal)
        NULL,                // Handle da task (não usado)
        0                    // Core 0 (WiFi/System)
    );

    // Cria a task de controle no Core 1 com PRIORIDADE ALTA
    xTaskCreatePinnedToCore(
        tankControlTask,     // Função da task
        "TankControlTask",   // Nome da task
        8192,                // Stack em bytes
        NULL,                // Parâmetros da task
        2,                   // Prioridade 2 (Alta - acima do loop padrão)
        NULL,                // Handle da task (não usado)
        1                    // Core 1 (App Core)
    );
}

void loop() {
  // Loop vazio pois o controle agora roda em task dedicada
  // Um delay longo aqui libera o Core 1 para a TankControlTask e outras tarefas do sistema
  vTaskDelay(pdMS_TO_TICKS(1000));
}
