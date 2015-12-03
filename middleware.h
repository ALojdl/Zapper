#ifndef _CONTROLER_H
#define _CONTROLER_H

#include <stdint.h>

typedef enum _middleware_error_t
{
    MIDDLEWARE_ERROR = -1,
    MIDDLEWARE_NO_ERROR
}middleware_error_t;

typedef int32_t (*changed_channel_callback_t)(uint8_t channel);

typedef struct _channel_info_t
{
    uint8_t channelNumber;
    uint8_t channelIndex;
    uint8_t teletextExist;
    uint8_t audioNumber;  
    uint8_t audioPID;
    uint8_t videoPID;  
    char date[11];
} channel_info_t;

void savePath(char *pathToConfigFile);
void initHardware();
void deinitHardware();
void channelDown();
void channelUp();
void goToChannel(uint16_t channel);
void volumeUp(uint8_t volumeStep);
void volumeDown(uint8_t volumeStep);
void muteVolume();
void registerChannelCallback(changed_channel_callback_t callbackFunction);
uint8_t getVolume();
channel_info_t getChannelInfo();

#endif // _CONTROLER_H
