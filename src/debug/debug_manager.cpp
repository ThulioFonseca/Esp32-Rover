#include "debug_manager.h"
#include "../config/config.h"
#include <Arduino.h>

DebugManager::DebugManager() : isEnabled(false), lastPrintTime(0), logHead(0), logTail(0), logCount(0) {}

void DebugManager::initialize() {
  lastPrintTime = millis();
  logf(LOG_LEVEL_INFO, "DebugManager inicializado");
}

void DebugManager::logf(LogLevel level, const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  String message(buf);
  
  const char* levelStr;
  switch (level) {
    case LOG_LEVEL_DEBUG: levelStr = "DEBUG"; break;
    case LOG_LEVEL_INFO:  levelStr = "INFO";  break;
    case LOG_LEVEL_WARN:  levelStr = "WARN";  break;
    case LOG_LEVEL_ERROR: levelStr = "ERROR"; break;
    default:              levelStr = "LOG";   break;
  }

  // Adiciona tempo simplificado (HH:MM:SS format ou apenas ms)
  unsigned long ms = millis();
  unsigned long s = ms / 1000;
  char tsBuf[32];
  snprintf(tsBuf, sizeof(tsBuf), "[%02lu:%02lu:%02lu]", (s / 3600), ((s % 3600) / 60), (s % 60));
  
  String finalMsg = String(tsBuf) + " [" + levelStr + "] " + message;

  // Usa isEnabled como controle principal da serial.
  if (isEnabled) {
    Serial.println(finalMsg);
  }

  // Insere no buffer circular
  logBuffer[logHead] = finalMsg;
  logHead = (logHead + 1) % MAX_LOG_LINES;
  
  if (logCount < MAX_LOG_LINES) {
    logCount++;
  } else {
    logTail = (logTail + 1) % MAX_LOG_LINES; // Sobrescreve o mais antigo
  }
}

String DebugManager::getLogs() {
  String output = "";
  if (logCount == 0) {
    return "Sem logs no momento.\n";
  }

  // Pre-aloca tamanho aproximado para eficiência
  output.reserve(logCount * 50);

  int current = logTail;
  for (int i = 0; i < logCount; i++) {
    output += logBuffer[current] + "\n";
    current = (current + 1) % MAX_LOG_LINES;
  }
  return output;
}

void DebugManager::clearLogs() {
  logHead = 0;
  logTail = 0;
  logCount = 0;
}

void DebugManager::setEnabled(bool enabled) {
    isEnabled = enabled;
}

bool DebugManager::isDebugEnabled() const {
    return isEnabled;
}

void DebugManager::printChannelData(const Types::ChannelData& channels) {
    if (!shouldPrint()) return;

    logf(LOG_LEVEL_DEBUG, "CH: T:%4d S:%4d | nT:%.3f nS:%.3f | AUX1:%4d AUX2:%4d",
         channels.throttle, channels.steering, channels.nThrottle, channels.nSteering,
         channels.aux[0], channels.aux[1]);
}

void DebugManager::printMotorCommands(const Types::MotorCommands& motors) {
    if (!shouldPrint()) return;

    logf(LOG_LEVEL_DEBUG, "MOT: L:%.3f R:%.3f | PWM: L:%4d R:%4d",
         motors.left, motors.right, motors.leftPWM, motors.rightPWM);

    updatePrintTime();
}

void DebugManager::printSystemStatus(Types::SystemState state) {
    const char* stateStr;
    switch (state) {
        case Types::INITIALIZING: stateStr = "INICIALIZANDO"; break;
        case Types::ARMING:       stateStr = "ARMANDO";       break;
        case Types::ARMED:        stateStr = "ARMADO";        break;
        case Types::TIMEOUT:      stateStr = "TIMEOUT";       break;
        case Types::ERROR:        stateStr = "ERRO";          break;
        default:                  stateStr = "DESCONHECIDO";  break;
    }

    logf(LOG_LEVEL_INFO, "*** ESTADO: %s ***", stateStr);
}

void DebugManager::printTimeout() {
    if (!shouldPrint()) return;

    logf(LOG_LEVEL_WARN, "TIMEOUT: Sinal iBUS perdido — motores em neutro");
    updatePrintTime();
}


bool DebugManager::shouldPrint() {
    return (millis() - lastPrintTime >= Config::DEBUG_INTERVAL_MS);
}

void DebugManager::updatePrintTime() {
    lastPrintTime = millis();
}
