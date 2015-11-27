#ifndef _CONTROLER_H
#define _CONTROLER_H

#include <stdint.h>

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

#endif // _CONTROLER_H
