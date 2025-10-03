#ifndef UTILS_H
#define UTILS_H

namespace Utils {
  int clampi(int value, int min, int max);
  float clampf(float value, float min, float max);
  float normalizeStick(int pulse);
  int denormalizeToEsc(float norm);
  bool isInRange(int value, int min, int max);
  unsigned long getElapsedTime(unsigned long startTime);
}

#endif