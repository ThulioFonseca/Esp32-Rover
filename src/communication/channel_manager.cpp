#include "channel_manager.h"
#include "../config/config.h"
#include "../utils/utils.h"
#include <Arduino.h>

ChannelManager::ChannelManager() : lastUpdateTime(0), isInitialized(false) {}

bool ChannelManager::initialize() {
  ibus.begin(Serial2, Config::IBUS_MODE);
  isInitialized = true;
  lastUpdateTime = millis();
  Serial.println("ChannelManager inicializado");
  return true;
}

void ChannelManager::update() {
  if (!isInitialized) return;
  
  updateChannelData();
}

void ChannelManager::updateChannelData() {
  int rawThrottle = ibus.readChannel(Types::CH_THROTTLE);
  int rawSteering = ibus.readChannel(Types::CH_STEERING);

  bool dataReceived = false;

  if (rawThrottle > 0) {
    channelData.throttle = Utils::clampi(rawThrottle, Config::PWM_MIN, Config::PWM_MAX);
    channelData.nThrottle = Utils::normalizeStick(channelData.throttle);
    dataReceived = true;
  } else {
    channelData.throttle = Config::PWM_MID;
    channelData.nThrottle = 0.0f;
  }

  if (rawSteering > 0) {
    channelData.steering = Utils::clampi(rawSteering, Config::PWM_MIN, Config::PWM_MAX);
    channelData.nSteering = Utils::normalizeStick(channelData.steering);
    dataReceived = true;
  } else {
    channelData.steering = Config::PWM_MID;
    channelData.nSteering = 0.0f;
  }

  // Read auxiliary channels
  for (int i = 0; i < 8; ++i) { // Assuming 8 auxiliary channels
    int rawAux = ibus.readChannel(Types::CH_AUX1 + i);
    if (rawAux > 0) {
      channelData.aux[i] = Utils::clampi(rawAux, Config::PWM_MIN, Config::PWM_MAX);
      dataReceived = true;
    } else {
      channelData.aux[i] = Config::PWM_MID;
    }
  }

  if (dataReceived) {
    channelData.isValid = true;
    channelData.lastUpdateTime = millis();
    lastUpdateTime = channelData.lastUpdateTime;
  }
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
  if (channel >= 2 && channel <= 9) return channelData.aux[channel - 2];
  return Config::PWM_MID;
}

float ChannelManager::getNormalizedChannel(int channel) const {
  if (channel == Types::CH_THROTTLE) return channelData.nThrottle;
  if (channel == Types::CH_STEERING) return channelData.nSteering;
  return 0.0f;
}