#include "debug_manager.h"
#include "../config/config.h"
#include <Arduino.h>

DebugManager::DebugManager() : isEnabled(true), lastPrintTime(0) {}

void DebugManager::initialize() {
  // Debug manager não precisa de inicialização especial
  // Serial já deve estar inicializado pelo controller principal
  lastPrintTime = millis();
}

void DebugManager::setEnabled(bool enabled) {
  isEnabled = enabled;
}

bool DebugManager::isDebugEnabled() const {
  return isEnabled;
}

void DebugManager::printChannelData(const Types::ChannelData& channels) {
  if (!isEnabled || !shouldPrint()) {
    return;
  }

  Serial.printf("CH: T:%4d S:%4d | nT:%.3f nS:%.3f",
                channels.throttle, channels.steering,
                channels.nThrottle, channels.nSteering);

  for (int i = 0; i < 8; ++i) { // Assuming 8 auxiliary channels
    Serial.printf(" | AUX%d:%4d", i + 1, channels.aux[i]);
  }
}

void DebugManager::printMotorCommands(const Types::MotorCommands& motors) {
  if (!isEnabled || !shouldPrint()) {
    return;
  }
  
  Serial.printf(" | MOT: L:%.3f R:%.3f | PWM: L:%4d R:%4d\n",
                motors.left, motors.right,
                motors.leftPWM, motors.rightPWM);
  
  updatePrintTime();
}

void DebugManager::printSystemStatus(Types::SystemState state) {
  if (!isEnabled) return;
  
  const char* stateStr;
  switch (state) {
    case Types::INITIALIZING: stateStr = "INICIALIZANDO"; break;
    case Types::ARMING:       stateStr = "ARMANDO"; break;
    case Types::ARMED:        stateStr = "ARMADO"; break;
    case Types::TIMEOUT:      stateStr = "TIMEOUT"; break;
    case Types::ERROR:        stateStr = "ERRO"; break;
    default:                  stateStr = "DESCONHECIDO"; break;
  }
  
  Serial.printf("*** ESTADO: %s ***\n", stateStr);
}

void DebugManager::printTimeout() {
  if (!isEnabled || !shouldPrint()) {
    return;
  }
  
  Serial.println("TIMEOUT: Sinal iBUS perdido - Motores em neutro");
  updatePrintTime();
}

void DebugManager::printError(const char* message) {
  if (!isEnabled) return;
  
  Serial.printf("ERRO: %s\n", message);
}

bool DebugManager::shouldPrint() {
  return (millis() - lastPrintTime >= Config::DEBUG_INTERVAL_MS);
}

void DebugManager::updatePrintTime() {
  lastPrintTime = millis();
}