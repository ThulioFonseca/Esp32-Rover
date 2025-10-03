#include "utils.h"
#include "../config/config.h"
#include <Arduino.h>

namespace Utils {
  int clampi(int value, int min, int max) {
    return (value < min) ? min : (value > max ? max : value);
  }

  float clampf(float value, float min, float max) {
    return (value < min) ? min : (value > max ? max : value);
  }

  float normalizeStick(int pulse) {
    int delta = pulse - Config::PWM_MID;
    
    if (abs(delta) <= Config::STICK_DEADZONE) {
      return 0.0f;
    }
    
    if (delta > 0) {
      float spanPos = static_cast<float>(
        Config::PWM_MAX - (Config::PWM_MID + Config::STICK_DEADZONE)
      );
      return static_cast<float>(delta - Config::STICK_DEADZONE) / spanPos;
    } else {
      float spanNeg = static_cast<float>(
        (Config::PWM_MID - Config::STICK_DEADZONE) - Config::PWM_MIN
      );
      return static_cast<float>(delta + Config::STICK_DEADZONE) / spanNeg;
    }
  }

  int denormalizeToEsc(float norm) {
    if (abs(norm) < 1e-6f) {
      return Config::ESC_NEUTRAL;
    }
    
    if (norm > 0.0f) {
      int outMin = Config::ESC_DEAD_HIGH + 1;
      int outMax = Config::PWM_MAX;
      return outMin + static_cast<int>(norm * (outMax - outMin));
    } else {
      float pos = -norm;
      int outMin = Config::PWM_MIN;
      int outMax = Config::ESC_DEAD_LOW - 1;
      return outMax - static_cast<int>(pos * (outMax - outMin));
    }
  }

  bool isInRange(int value, int min, int max) {
    return (value >= min && value <= max);
  }

  unsigned long getElapsedTime(unsigned long startTime) {
    return millis() - startTime;
  }
}