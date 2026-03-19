#ifndef UTILS_H
#define UTILS_H

namespace Utils {
  int clampi(int value, int min, int max);
  float normalizeStick(int pulse);
  int denormalizeToEsc(float norm);
}

#endif