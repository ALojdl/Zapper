#include <stdio.h>
#include <string.h>
#include "middleware.h"
#include "graphics.h"
#include "remote.h"
#include "globals.h"

#define VOLUME_STEP             10

static uint8_t volume;
static channel_info_t channelInfo;
static info_data_t infoBarData;

void copyValues(channel_info_t *channelStruct, info_data_t *infoStruct)
{
    infoStruct->channelNumber = channelStruct->channelNumber;
    infoStruct->teletextExist = channelStruct->teletextExist;
    strcpy(infoStruct->date, channelStruct->date);
}

int32_t channelChanged(uint8_t channel)
{
    channelInfo = getChannelInfo();
    copyValues(&channelInfo, &infoBarData);
    drawInfoBar(infoBarData);
    return 0;
}

int32_t callbackFunction(uint16_t keyPressed)
{
    switch (keyPressed)
    {
        case KEYCODE_P_MINUS: 
        {
            channelDown();
            break;
        }            
        case KEYCODE_P_PLUS:
        { 
            channelUp();
            break;
        }    
        case KEYCODE_V_PLUS: 
        {
            volumeUp(VOLUME_STEP);
            volume = getVolume();
            drawVolume(volume/VOLUME_STEP);
            break;
        }    
        case KEYCODE_V_MINUS: 
        {
            volumeDown(VOLUME_STEP);
            volume = getVolume();
            drawVolume(volume/VOLUME_STEP);
            break;
        }    
        case KEYCODE_MUTE: 
        {
            muteVolume();
            volume = getVolume();
            drawVolume(volume/VOLUME_STEP);
            break;
        }    
        case KEYCODE_EXIT: 
        {
            deinitHardware();
            break;
        }    
        case KEYCODE_INFO: 
        {
            channelInfo = getChannelInfo();
            copyValues(&channelInfo, &infoBarData);
            drawInfoBar(infoBarData);
            break;   
        }                                                 
        default:
        {
            goToChannel(keyPressed - 1);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[])
{    
    if (argc == 2)
    {
        savePath(argv[1]);
    }
    else
    {
        printf("ERROR: Wrong number of arguments when calling this app.\n");
    }
    
    // Initialize. 
    initHardware();  
    registerChannelCallback(channelChanged);
    initRemote(callbackFunction);
    
    // Deinitialize.
    deinitRemote();     
    
    return 0;
}
