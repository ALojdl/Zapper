#include <stdio.h>
#include "controler.h"
#include "graphics.h"
#include "remote.h"

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
            volumeUp();
            break;
        }    
        case KEYCODE_V_MINUS: 
        {
            volumeDown();
            break;
        }    
        case KEYCODE_MUTE: 
        {
            muteVolume();
            break;
        }    
        case KEYCODE_EXIT: 
        {
            deinitHardware();
            break;
        }    
        case KEYCODE_INFO: 
        {
            getInfo();
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
    initRemote(callbackFunction);
    
    // Deinitialize.
    deinitRemote();     
    
    return 0;
}
