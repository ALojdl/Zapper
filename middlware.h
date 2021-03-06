#ifndef _CONTROLER_H
#define _CONTROLER_H

#include <stdint.h>
#include "globals.h"

typedef int32_t (*changed_channel_callback_t)(uint8_t channel);

void savePath(char *pathToConfigFile);
void initHardware();
void deinitHardware();
void channelDown();
void channelUp();
void goToChannel(uint16_t channel);
void volumeUp();
void volumeDown();
void muteVolume();
void getInfo();
void registerChannelCallback(changed_channel_callback_t callbackFunction);

#endif // _CONTROLER_H
