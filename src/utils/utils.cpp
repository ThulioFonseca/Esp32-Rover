#include "utils.h"
#include "../config/config.h"
#include <Arduino.h>

namespace Utils {

int clampi(int value, int min, int max) {
    return (value < min) ? min : (value > max ? max : value);
}

/**
 * Converte um pulso PWM (1000–2000 µs) em um valor normalizado [-1.0, +1.0].
 *
 * Aplica deadzone simétrica ao redor de PWM_MID antes de normalizar,
 * garantindo que jitter residual do stick não gere movimento indesejado.
 * O span positivo e negativo são calculados separadamente para preservar
 * a linearidade mesmo que PWM_MID não seja exatamente o centro do range.
 */
float normalizeStick(int pulse) {
    int delta = pulse - Config::PWM_MID;

    if (abs(delta) <= Config::STICK_DEADZONE) return 0.0f;

    if (delta > 0) {
        float span = static_cast<float>(Config::PWM_MAX - (Config::PWM_MID + Config::STICK_DEADZONE));
        return static_cast<float>(delta - Config::STICK_DEADZONE) / span;
    } else {
        float span = static_cast<float>((Config::PWM_MID - Config::STICK_DEADZONE) - Config::PWM_MIN);
        return static_cast<float>(delta + Config::STICK_DEADZONE) / span;
    }
}

/**
 * Converte um valor normalizado [-1.0, +1.0] em pulso PWM para o ESC,
 * saltando a região de deadband (ESC_DEAD_LOW–ESC_DEAD_HIGH) onde o ESC
 * não responde. Zero retorna ESC_NEUTRAL (centro do deadband).
 *
 * Positivo → [ESC_DEAD_HIGH+1, PWM_MAX]
 * Negativo → [PWM_MIN, ESC_DEAD_LOW-1]
 */
int denormalizeToEsc(float norm) {
    if (fabsf(norm) < 1e-6f) return Config::ESC_NEUTRAL;

    if (norm > 0.0f) {
        int outMin = Config::ESC_DEAD_HIGH + 1;
        int outMax = Config::PWM_MAX;
        return outMin + static_cast<int>(norm * (outMax - outMin));
    } else {
        float pos  = -norm;
        int outMin = Config::PWM_MIN;
        int outMax = Config::ESC_DEAD_LOW - 1;
        return outMax - static_cast<int>(pos * (outMax - outMin));
    }
}

} // namespace Utils
