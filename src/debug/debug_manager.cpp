#include "debug_manager.h"
#include "../config/config.h"
#include "../utils/platform.h"
#include <Arduino.h>

DebugManager::DebugManager() : isEnabled(false), _prevNThrottle(0.0f), _prevNSteering(0.0f), logHead(0), logTail(0), logCount(0), serialMutex(NULL) {}

void DebugManager::initialize() {
  if (serialMutex == NULL) {
    serialMutex = xSemaphoreCreateMutex();
  }
  logf(LOG_LEVEL_INFO, "DebugManager inicializado");
}

void DebugManager::logf(LogLevel level, const char* format, ...) {
  char msgBuf[MAX_MSG_LEN];
  va_list args;
  va_start(args, format);
  vsnprintf(msgBuf, sizeof(msgBuf), format, args);
  va_end(args);

  const char* levelStr;
  switch (level) {
    case LOG_LEVEL_DEBUG: levelStr = "DEBUG"; break;
    case LOG_LEVEL_INFO:  levelStr = "INFO";  break;
    case LOG_LEVEL_WARN:  levelStr = "WARN";  break;
    case LOG_LEVEL_ERROR: levelStr = "ERROR"; break;
    default:              levelStr = "LOG";   break;
  }

  unsigned long ms = millis();
  unsigned long s = ms / 1000;

  // Formata a linha final diretamente em char[] (zero alocação heap)
  char finalMsg[MAX_LOG_LINE_LEN];
  snprintf(finalMsg, sizeof(finalMsg), "[%02lu:%02lu:%02lu] [%s] %s",
           (s / 3600), ((s % 3600) / 60), (s % 60), levelStr, msgBuf);

  // Mutex protege Serial E o buffer circular (evita race condition entre cores)
  if (serialMutex != NULL && xSemaphoreTake(serialMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    if (isEnabled) {
      ets_printf("%s\n", finalMsg);
    }

    // Insere no buffer circular (protegido pelo mesmo mutex)
    strncpy(logBuffer[logHead], finalMsg, MAX_LOG_LINE_LEN - 1);
    logBuffer[logHead][MAX_LOG_LINE_LEN - 1] = '\0';
    logHead = (logHead + 1) % MAX_LOG_LINES;

    if (logCount < MAX_LOG_LINES) {
      logCount++;
    } else {
      logTail = (logTail + 1) % MAX_LOG_LINES;
    }

    xSemaphoreGive(serialMutex);
  }
  // Mutex não disponível imediatamente — mensagem descartada (melhor que corromper estado)
}

String DebugManager::getLogs() {
  if (serialMutex == NULL) return "Mutex não inicializado.\n";

  String output;
  if (xSemaphoreTake(serialMutex, 0) == pdTRUE) {
    if (logCount == 0) {
      xSemaphoreGive(serialMutex);
      return "Sem logs no momento.\n";
    }

    output.reserve(logCount * MAX_LOG_LINE_LEN);
    int current = logTail;
    for (int i = 0; i < logCount; i++) {
      output += logBuffer[current];
      output += '\n';
      current = (current + 1) % MAX_LOG_LINES;
    }
    xSemaphoreGive(serialMutex);
  } else {
    return "Log buffer ocupado.\n";
  }
  return output;
}

size_t DebugManager::writeLogsToBuffer(char* buf, size_t bufSize) {
  if (serialMutex == NULL || buf == NULL || bufSize == 0) return 0;

  size_t written = 0;
  if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    int current = logTail;
    for (int i = 0; i < logCount; i++) {
      int n = snprintf(buf + written, bufSize - written, "%s\n", logBuffer[current]);
      if (n > 0 && (size_t)n < bufSize - written) {
        written += n;
      } else {
        break; // buffer cheio
      }
      current = (current + 1) % MAX_LOG_LINES;
    }
    xSemaphoreGive(serialMutex);
  }
  return written;
}

void DebugManager::clearLogs() {
  if (serialMutex != NULL && xSemaphoreTake(serialMutex, 0) == pdTRUE) {
    logHead = 0;
    logTail = 0;
    logCount = 0;
    xSemaphoreGive(serialMutex);
  }
}

void DebugManager::logSerial(LogLevel level, const char* format, ...) {
  if (!isEnabled) return;

  char msgBuf[MAX_MSG_LEN];
  va_list args;
  va_start(args, format);
  vsnprintf(msgBuf, sizeof(msgBuf), format, args);
  va_end(args);

  const char* levelStr;
  switch (level) {
    case LOG_LEVEL_DEBUG: levelStr = "DEBUG"; break;
    case LOG_LEVEL_INFO:  levelStr = "INFO";  break;
    case LOG_LEVEL_WARN:  levelStr = "WARN";  break;
    case LOG_LEVEL_ERROR: levelStr = "ERROR"; break;
    default:              levelStr = "LOG";   break;
  }

  unsigned long ms = millis();
  unsigned long s = ms / 1000;

  char finalMsg[MAX_LOG_LINE_LEN];
  snprintf(finalMsg, sizeof(finalMsg), "[%02lu:%02lu:%02lu] [%s] %s",
           (s / 3600), ((s % 3600) / 60), (s % 60), levelStr, msgBuf);

  if (serialMutex != NULL && xSemaphoreTake(serialMutex, 0) == pdTRUE) {
    ets_printf("%s\n", finalMsg);
    xSemaphoreGive(serialMutex);
  }
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

