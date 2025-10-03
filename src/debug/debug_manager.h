#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include "../types/types.h"

class DebugManager {
private:
  bool isEnabled;
  unsigned long lastPrintTime;

public:
  DebugManager();
  
  void initialize();
  void setEnabled(bool enabled);
  bool isDebugEnabled() const;
  
  void printChannelData(const Types::ChannelData& channels);
  void printMotorCommands(const Types::MotorCommands& motors);
  void printSystemStatus(Types::SystemState state);
  void printTimeout();
  void printError(const char* message);
  
private:
  bool shouldPrint();
  void updatePrintTime();
};

#endif