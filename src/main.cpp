#include "controllers/tank_controller.h"

TankController tankController;

void setup() {
  if (!tankController.initialize()) {
    Serial.println("ERRO: Falha na inicialização!");
    while(1) {
      delay(1000);
    }
  }
}

void loop() {
  tankController.update();
  delay(5);
}