#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <IBusBM.h>
#include "../types/types.h"

class ChannelManager {
private:
  IBusBM ibus;
  Types::ChannelData channelData;
  volatile unsigned long lastUpdateTime;
  bool isInitialized;
  uint8_t  lastFrameCount; // último valor de ibus.cnt_rec (contador de 8 bits da biblioteca)
  uint32_t totalFrames;    // frames iBUS válidos desde o boot (telemetria)

public:
  ChannelManager();
  
  bool initialize();
  void poll();   // Processa bytes recebidos do iBUS — chamado a 1 kHz pela ibusPollTask
  void update(); // Consome o frame mais recente — chamado pela tankControlTask
  bool isDataValid() const;
  bool hasTimeout() const;
  
  const Types::ChannelData& getChannelData() const;
  int readChannel(int channel) const;
  float getNormalizedChannel(int channel) const;
  uint32_t getFrameCount() const;
  
private:
  void updateChannelData();
};

#endif