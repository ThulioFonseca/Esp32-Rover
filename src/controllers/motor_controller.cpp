#include "motor_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>

MotorController::MotorController() : isInitialized(false), lastUpdateTime(0) {}

bool MotorController::initialize() {
  escLeft.attach(Pins::ESC_LEFT);
  escRight.attach(Pins::ESC_RIGHT);
  
  isInitialized = true;
  Serial.println("MotorController inicializado");
  return true;
}

void MotorController::update(float throttle, float steering) {
  if (!isInitialized) return;
  
  calculateMotorCommands(throttle, steering);
  normalizeMotorCommands();
  updatePWMOutputs();
  
  lastUpdateTime = millis();
}

void MotorController::calculateMotorCommands(float throttle, float steering) {
  commands.left = throttle + steering;
  commands.right = throttle - steering;
}

void MotorController::normalizeMotorCommands() {
  float maxAbs = max(abs(commands.left), abs(commands.right));
  
  if (maxAbs > 1.0f) {
    commands.left /= maxAbs;
    commands.right /= maxAbs;
  }
}

void MotorController::updatePWMOutputs() {
  commands.leftPWM = Utils::denormalizeToEsc(commands.left);
  commands.rightPWM = Utils::denormalizeToEsc(commands.right);
  
  setMotorOutputs(commands.leftPWM, commands.rightPWM);
}

void MotorController::setMotorOutputs(int leftPWM, int rightPWM) {
  escLeft.writeMicroseconds(leftPWM);
  escRight.writeMicroseconds(rightPWM);
}

void MotorController::setNeutral() {
  commands.left = 0.0f;
  commands.right = 0.0f;
  commands.leftPWM = Config::ESC_NEUTRAL;
  commands.rightPWM = Config::ESC_NEUTRAL;
  
  setMotorOutputs(commands.leftPWM, commands.rightPWM);
}

void MotorController::performArmingSequence() {
  Serial.println("Iniciando sequência de armamento...");
  unsigned long startTime = millis();
  
  while (millis() - startTime < Config::ARMING_TIME_MS) {
    setMotorOutputs(Config::ESC_NEUTRAL, Config::ESC_NEUTRAL);
    delay(20);
    yield();
  }
  
  Serial.println("Sequência de armamento concluída.");
}

const Types::MotorCommands& MotorController::getCommands() const {
  return commands;
}