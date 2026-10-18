#include "motor_controller.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>
#include <cmath>

MotorController::MotorController() : isInitialized(false), leftActive(false), rightActive(false) {}

bool MotorController::initialize() {
    if (escLeft.attach(Pins::ESC_LEFT) == -1) {
        return false;
    }
    if (escRight.attach(Pins::ESC_RIGHT) == -1) {
        escLeft.detach();
        return false;
    }

    // Resolução do PWM: o padrão do ESP32Servo é 10 bits a 50 Hz (20000 µs / 1024 ≈ 19,5 µs por
    // passo), o que deixa só ~21 degraus entre o fim do deadband e o máximo — cada degrau é um
    // tranco visível em velocidade baixa. 16 bits dá ~0,3 µs por passo.
    // Precisa vir DEPOIS do attach: attach() num servo novo restaura a largura padrão de 10 bits;
    // com o pino já anexado, setTimerWidth() refaz o canal LEDC com a nova resolução.
    escLeft.setTimerWidth(Config::ESC_PWM_TIMER_WIDTH);
    escRight.setTimerWidth(Config::ESC_PWM_TIMER_WIDTH);
    setMotorOutputs(Config::ESC_NEUTRAL, Config::ESC_NEUTRAL); // reescreve o duty na nova escala

    isInitialized = true;
    return true;
}

void MotorController::update(float throttle, float steering) {
    if (!isInitialized) return;

    calculateMotorCommands(throttle, steering);
    normalizeMotorCommands();
    updatePWMOutputs();
}

// Tank mixing: left = throttle + steering, right = throttle - steering.
// Deadband de saída com histerese: comandos muito pequenos são zerados para evitar motor twitch
// causado por ruído residual do transmissor RC após a deadzone de input. O motor só liga acima de
// MOTOR_OUTPUT_DEADBAND e só desliga abaixo de MOTOR_OUTPUT_RELEASE — sem a faixa entre os dois,
// segurar o stick no limiar faz o ruído de ±1–2 µs do iBUS ligar/desligar o motor (trancos).
static constexpr float MOTOR_OUTPUT_DEADBAND = 0.03f;
static constexpr float MOTOR_OUTPUT_RELEASE  = 0.02f;

static float applyOutputDeadband(float cmd, bool& active) {
    float threshold = active ? MOTOR_OUTPUT_RELEASE : MOTOR_OUTPUT_DEADBAND;
    active = fabsf(cmd) >= threshold;
    return active ? cmd : 0.0f;
}

void MotorController::calculateMotorCommands(float throttle, float steering) {
    commands.left  = applyOutputDeadband(throttle + steering, leftActive);
    commands.right = applyOutputDeadband(throttle - steering, rightActive);
}

// Mantém os comandos em [-1.0, 1.0] preservando a relação proporcional entre eles.
void MotorController::normalizeMotorCommands() {
    float maxAbs = max(abs(commands.left), abs(commands.right));
    if (maxAbs > 1.0f) {
        commands.left  /= maxAbs;
        commands.right /= maxAbs;
    }
}

void MotorController::updatePWMOutputs() {
    commands.leftPWM  = Utils::denormalizeToEsc(commands.left);
    commands.rightPWM = Utils::denormalizeToEsc(commands.right);
    setMotorOutputs(commands.leftPWM, commands.rightPWM);
}

void MotorController::setMotorOutputs(int leftPWM, int rightPWM) {
    escLeft.writeMicroseconds(leftPWM);
    escRight.writeMicroseconds(rightPWM);
}

void MotorController::setNeutral() {
    leftActive        = false;
    rightActive       = false;
    commands.left     = 0.0f;
    commands.right    = 0.0f;
    commands.leftPWM  = Config::ESC_NEUTRAL;
    commands.rightPWM = Config::ESC_NEUTRAL;
    setMotorOutputs(commands.leftPWM, commands.rightPWM);
}

// Mantém o sinal neutro por ARMING_TIME_MS para que os ESCs concluam a sequência de boot.
void MotorController::performArmingSequence() {
    unsigned long startTime = millis();
    while (millis() - startTime < Config::ARMING_TIME_MS) {
        setMotorOutputs(Config::ESC_NEUTRAL, Config::ESC_NEUTRAL);
        delay(20);
        yield();
    }
}

const Types::MotorCommands& MotorController::getCommands() const {
    return commands;
}
