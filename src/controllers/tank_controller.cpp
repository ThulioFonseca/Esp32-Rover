#include "tank_controller.h"
#include "../config/config.h"
#include "../utils/utils.h"
#include <Arduino.h>

TankController::TankController() 
  : currentState(Types::INITIALIZING), 
    lastControlTime(0), 
    systemArmed(false) {}

bool TankController::initialize() {
  Serial.begin(Config::SERIAL_BAUD);
  Serial.println("=== Inicializando TankController ===");
  
  currentState = Types::INITIALIZING;
  
  // Inicializar Debug Manager primeiro
  debugManager.initialize();
  debugManager.setEnabled(true);
  
  // Inicializar Channel Manager
  if (!channelManager.initialize()) {
    debugManager.printError("Falha ao inicializar ChannelManager");
    currentState = Types::ERROR;
    return false;
  }
  
  // Inicializar Motor Controller
  if (!motorController.initialize()) {
    debugManager.printError("Falha ao inicializar MotorController");
    currentState = Types::ERROR;
    return false;
  }
  
  // Realizar sequência de armamento
  currentState = Types::ARMING;
  debugManager.printSystemStatus(currentState);
  motorController.performArmingSequence();
  
  // Sistema pronto
  currentState = Types::ARMED;
  systemArmed = true;
  lastControlTime = millis();
  
  debugManager.printSystemStatus(currentState);
  Serial.println("=== Sistema Inicializado com Sucesso ===");
  
  return true;
}

void TankController::update() {
  // Yield para watchdog
  yield();
  
  // Verificar timing de controle
  if (!shouldUpdate()) {
    return;
  }
  
  lastControlTime = millis();
  
  // Atualizar dados dos canais
  channelManager.update();
  
  // Atualizar estado do sistema
  updateState();
  
  // Processar baseado no estado atual
  switch (currentState) {
    case Types::ARMED:
      updateSystem();
      break;
      
    case Types::TIMEOUT:
      handleTimeout();
      break;
      
    case Types::ERROR:
      // Manter motores em neutro em caso de erro
      motorController.setNeutral();
      break;
      
    default:
      // Estados de inicialização - não fazer nada
      break;
  }
}

void TankController::setDebugMode(bool enabled) {
  debugManager.setEnabled(enabled);
}

const Types::ChannelData& TankController::getChannelData() const {
  return channelManager.getChannelData();
}

const Types::MotorCommands& TankController::getMotorCommands() const {
  return motorController.getCommands();
}

Types::SystemState TankController::getSystemState() const {
  return currentState;
}

bool TankController::isSystemArmed() const {
  return systemArmed;
}

void TankController::updateSystem() {
  // Verificar se há dados válidos
  if (!channelManager.isDataValid()) {
    return;
  }
  
  // Processar controles
  processControls();
  
  // Debug output se habilitado
  if (debugManager.isDebugEnabled()) {
    debugManager.printChannelData(channelManager.getChannelData());
    debugManager.printMotorCommands(motorController.getCommands());
  }
}

void TankController::handleTimeout() {
  // Colocar motores em neutro
  motorController.setNeutral();
  
  // Imprimir debug se habilitado
  if (debugManager.isDebugEnabled()) {
    debugManager.printTimeout();
  }
  
  // Verificar se o sinal voltou
  if (channelManager.isDataValid() && !channelManager.hasTimeout()) {
    currentState = Types::ARMED;
    debugManager.printSystemStatus(currentState);
  }
}

void TankController::processControls() {
  const Types::ChannelData& channels = channelManager.getChannelData();
  
  // Obter valores normalizados dos controles
  float throttle = channels.nThrottle;
  float steering = channels.nSteering;
  
  // Atualizar controlador de motores
  motorController.update(throttle, steering);
}

bool TankController::shouldUpdate() {
  unsigned long now = millis();
  return (now - lastControlTime >= Config::CONTROL_INTERVAL_MS);
}

void TankController::updateState() {
  Types::SystemState previousState = currentState;
  
  // Verificar timeout apenas se sistema estiver armado
  if (currentState == Types::ARMED) {
    if (channelManager.hasTimeout()) {
      currentState = Types::TIMEOUT;
    }
  } 
  // Se estava em timeout, verificar se recuperou
  else if (currentState == Types::TIMEOUT) {
    if (!channelManager.hasTimeout() && channelManager.isDataValid()) {
      currentState = Types::ARMED;
    }
  }
  
  // Imprimir mudança de estado se necessário
  if (previousState != currentState && debugManager.isDebugEnabled()) {
    debugManager.printSystemStatus(currentState);
  }
}