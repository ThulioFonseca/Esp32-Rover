#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include "../types/types.h"
#include "freertos/semphr.h"

class DebugManager {
private:
  volatile bool isEnabled;

public:
  DebugManager();
  
  void initialize();
  void setEnabled(bool enabled);
  bool isDebugEnabled() const;
  
  enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
  };

  void logf(LogLevel level, const char* format, ...);
  // Log de controle baseado em mudança — só registra quando valores mudam significativamente
  void printControlState(const Types::ChannelData& channels, const Types::MotorCommands& motors);
  void printSystemStatus(Types::SystemState state);

  // Sistema de Log em RAM (Buffer Circular)
  String getLogs();
  void clearLogs();
  
private:
  float _prevNThrottle;
  float _prevNSteering;
  static constexpr float CTRL_LOG_THRESHOLD = 0.05f; // 5% de mudança para gerar log

  static const int MAX_LOG_LINES = 30;
  String logBuffer[MAX_LOG_LINES];
  int logHead;
  int logTail;
  int logCount;
  SemaphoreHandle_t serialMutex;
};

#endif