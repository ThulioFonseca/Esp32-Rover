#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include "../types/types.h"

class DebugManager {
private:
  volatile bool isEnabled;
  unsigned long lastPrintTime;

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
  // Manter retrocompatibilidade por enquanto
  void printChannelData(const Types::ChannelData& channels);
  void printMotorCommands(const Types::MotorCommands& motors);
  void printSystemStatus(Types::SystemState state);
  void printTimeout();

  // Sistema de Log em RAM (Buffer Circular)
  String getLogs();
  void clearLogs();
  
private:
  bool shouldPrint();
  void updatePrintTime();

  static const int MAX_LOG_LINES = 30;
  String logBuffer[MAX_LOG_LINES];
  int logHead;
  int logTail;
  int logCount;
};

#endif