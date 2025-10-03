#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <ESP32Servo.h>
#include "../types/types.h"

class MotorController {
private:
  Servo escLeft, escRight;
  Types::MotorCommands commands;
  bool isInitialized;
  unsigned long lastUpdateTime;

public:
  MotorController();
  
  bool initialize();
  void update(float throttle, float steering);
  void setNeutral();
  void performArmingSequence();
  
  const Types::MotorCommands& getCommands() const;
  
private:
  void calculateMotorCommands(float throttle, float steering);
  void normalizeMotorCommands();
  void updatePWMOutputs();
  void setMotorOutputs(int leftPWM, int rightPWM);
};

#endif