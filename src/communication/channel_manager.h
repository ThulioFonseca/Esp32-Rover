#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <IBusBM.h>
#include "../types/types.h"

class ChannelManager {
private:
  IBusBM ibus;
  Types::ChannelData channelData;
  unsigned long lastUpdateTime;
  bool isInitialized;

public:
  ChannelManager();
  
  bool initialize();
  void update();
  bool isDataValid() const;
  bool hasTimeout() const;
  
  const Types::ChannelData& getChannelData() const;
  int readChannel(int channel) const;
  float getNormalizedChannel(int channel) const;
  
private:
  void updateChannelData();
  void resetChannelData();
};

#endif