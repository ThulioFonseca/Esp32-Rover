#ifndef TANK_CONTROLLER_H
#define TANK_CONTROLLER_H

#include "../types/types.h"
#include "../communication/channel_manager.h"
#include "../controllers/motor_controller.h"
#include "../debug/debug_manager.h"
#include <SPIFFS.h>

class TankController {
private:
  ChannelManager channelManager;
  MotorController motorController;
  DebugManager debugManager;
  
  Types::SystemState currentState;
  bool systemArmed;

public:
  TankController();
  
  bool initialize();
  void update(); // Chamado pela task RTOS em frequência fixa
  void setDebugMode(bool enabled);
  void setSystemArmed(bool armed);
  
  // Getters para monitoramento
  const Types::ChannelData& getChannelData() const;
  const Types::MotorCommands& getMotorCommands() const;
  Types::SystemState getSystemState() const;
  bool isSystemArmed() const;

private:
  void updateSystem();
  void handleTimeout();
  void processControls();
  void updateState();
};

#endif