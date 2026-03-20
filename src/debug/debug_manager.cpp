#include "debug_manager.h"
#include "../config/config.h"
#include <Arduino.h>

DebugManager::DebugManager() : isEnabled(false), logHead(0), logTail(0), logCount(0), serialMutex(NULL), _prevNThrottle(0.0f), _prevNSteering(0.0f) {}

void DebugManager::initialize() {
  if (serialMutex == NULL) {
    serialMutex = xSemaphoreCreateMutex();
  }
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

  // Serial protegida por mutex — evita intercalação de saídas entre Core 0 e Core 1.
  if (isEnabled && serialMutex != NULL) {
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      Serial.println(finalMsg);
      xSemaphoreGive(serialMutex);
    }
    // Se o mutex não puder ser adquirido em 10 ms, a mensagem segue apenas no buffer circular.
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

void DebugManager::printControlState(const Types::ChannelData& ch, const Types::MotorCommands& mot) {
    if (fabsf(ch.nThrottle - _prevNThrottle) < CTRL_LOG_THRESHOLD &&
        fabsf(ch.nSteering - _prevNSteering) < CTRL_LOG_THRESHOLD) return;

    _prevNThrottle = ch.nThrottle;
    _prevNSteering = ch.nSteering;

    logf(LOG_LEVEL_DEBUG, "CTRL: nT:%+.2f nS:%+.2f | L:%+.2f R:%+.2f [%d/%d]",
         ch.nThrottle, ch.nSteering, mot.left, mot.right, mot.leftPWM, mot.rightPWM);
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

