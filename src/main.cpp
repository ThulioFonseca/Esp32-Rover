#include "controllers/tank_controller.h"
#include "web/web_server_manager.h"

TankController tankController;
WebServerManager webServer("ESP32-ROVER", "rover1234");


void webServerTask(void* pvParameters) {
    Serial.println("[INFO] WebServer rodando na task dedicada");
    
    // Inicializa o servidor
    if (!webServer.begin()) {
        Serial.println("[ERRO] Falha ao iniciar servidor!");
        vTaskDelete(NULL);
        return;
    }

    // Loop infinito da task (AsyncWebServer não bloqueia, mas a task precisa existir)
    while (true) {
        // Apenas para não terminar a task
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void setup() {
  if (!tankController.initialize()) {
    Serial.println("ERRO: Falha na inicialização!");
    while(1) {
      delay(1000);
    }
  }
    // Cria a task para rodar no Core 1
    xTaskCreatePinnedToCore(
        webServerTask,       // Função da task
        "WebServerTask",     // Nome da task
        8192,                // Stack em bytes
        NULL,                // Parâmetros da task
        1,                   // Prioridade da task
        NULL,                // Handle da task (não usado)
        1                    // Core (0 ou 1)
    );
}

void loop() {
  tankController.update();
  delay(250);
}