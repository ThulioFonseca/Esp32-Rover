#include "channel_manager.h"
#include "../config/config.h"
#include "../config/pins.h"
#include "../utils/utils.h"
#include <Arduino.h>

ChannelManager::ChannelManager()
    : lastUpdateTime(0), isInitialized(false), lastFrameCount(0), totalFrames(0) {}

bool ChannelManager::initialize() {
    // IBUSBM_NOTIMER: sem timer de hardware. A ISR da biblioteca chamaria Serial2.available()/read()
    // em contexto de interrupção (não suportado pelo HardwareSerial do core Arduino) a cada 1 ms
    // no Core 0. O parsing passa a rodar em poll(), chamado pela ibusPollTask (Core 1, 1 kHz).
    ibus.begin(Serial2, IBUSBM_NOTIMER, Pins::IBUS_RX, Pins::IBUS_TX);
    lastFrameCount = ibus.cnt_rec;
    isInitialized = true;
    lastUpdateTime = millis();
    return true;
}

// Chamado a cada 1 ms pela ibusPollTask. A IBusBM separa frames pelo intervalo de >3 ms entre
// bytes (medido com millis() no momento do processamento), por isso precisa ser chamada com
// frequência — processar o buffer em rajadas a 50 Hz faria a biblioteca descartar frames.
void ChannelManager::poll() {
    if (!isInitialized) return;
    ibus.loop();
}

void ChannelManager::update() {
    if (!isInitialized) return;

    // A IBusBM mantém os últimos valores de canal quando os frames param de chegar.
    // Só o contador de frames válidos (cnt_rec) indica dado novo — é ele que alimenta o failsafe.
    uint8_t frameCount = ibus.cnt_rec;
    if (frameCount != lastFrameCount) {
        totalFrames += (uint8_t)(frameCount - lastFrameCount);
        lastFrameCount = frameCount;
        updateChannelData();
    }

    if (hasTimeout()) {
        channelData.isValid = false;
    }
}

void ChannelManager::updateChannelData() {
    int rawThrottle = ibus.readChannel(Types::CH_THROTTLE);
    int rawSteering = ibus.readChannel(Types::CH_STEERING);

    bool dataReceived = false;

    if (rawThrottle > 0) {
        channelData.throttle  = Utils::clampi(rawThrottle, Config::PWM_MIN, Config::PWM_MAX);
        channelData.nThrottle = Utils::normalizeStick(channelData.throttle);
        dataReceived = true;
    } else {
        channelData.throttle  = Config::PWM_MID;
        channelData.nThrottle = 0.0f;
    }

    if (rawSteering > 0) {
        channelData.steering  = Utils::clampi(rawSteering, Config::PWM_MIN, Config::PWM_MAX);
        channelData.nSteering = Utils::normalizeStick(channelData.steering);
        dataReceived = true;
    } else {
        channelData.steering  = Config::PWM_MID;
        channelData.nSteering = 0.0f;
    }

    for (int i = 0; i < 8; ++i) {
        int rawAux = ibus.readChannel(Types::CH_AUX1 + i);
        if (rawAux > 0) {
            channelData.aux[i] = Utils::clampi(rawAux, Config::PWM_MIN, Config::PWM_MAX);
            dataReceived = true;
        } else {
            channelData.aux[i] = Config::PWM_MID;
        }
    }

    // Chamado apenas quando um frame novo chegou (cnt_rec mudou).
    if (dataReceived) {
        channelData.isValid       = true;
        channelData.lastUpdateTime = millis();
        lastUpdateTime            = channelData.lastUpdateTime;
    }
}

uint32_t ChannelManager::getFrameCount() const {
    return totalFrames;
}

bool ChannelManager::isDataValid() const {
    return channelData.isValid;
}

bool ChannelManager::hasTimeout() const {
    return (millis() - lastUpdateTime > Config::IBUS_TIMEOUT_MS);
}

const Types::ChannelData& ChannelManager::getChannelData() const {
    return channelData;
}

int ChannelManager::readChannel(int channel) const {
    if (channel == Types::CH_THROTTLE) return channelData.throttle;
    if (channel == Types::CH_STEERING) return channelData.steering;
    if (channel >= 2 && channel <= 9)  return channelData.aux[channel - 2];
    return Config::PWM_MID;
}

float ChannelManager::getNormalizedChannel(int channel) const {
    if (channel == Types::CH_THROTTLE) return channelData.nThrottle;
    if (channel == Types::CH_STEERING) return channelData.nSteering;
    return 0.0f;
}
