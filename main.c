#include <stdio.h>

#include "parser.h"
#include "tundem.h"
#include "remote.h"

int32_t callbackFunction(uint16_t keyPressed)
{
    switch (keyPressed)
    {
        case KEYCODE_P_MINUS: 
            printf("CALL: channelDown()\n");
            break;
        case KEYCODE_P_PLUS: 
            printf("CALL: channelUp()\n");
            break;
        case KEYCODE_V_PLUS: 
            printf("CALL: volumeUp()\n");
            break;
        case KEYCODE_V_MINUS: 
            printf("CALL: volumeDown()\n");
            break;
        case KEYCODE_MUTE: 
            printf("CALL: muteVolume()\n");
            break;
        case KEYCODE_EXIT: 
            printf("CALL: closeControler()\n");
            break;
        case KEYCODE_INFO: 
            printf("CALL: getInfo()\n");
            break;                                                
        default:
            printf("CALL: goToChannel()\n");
    }
    
    return 0;
}

int main()
{
    init_data_t data;
    
    getConfiguration("./config.ini", &data);
    
    /*
    // Print configuration. 
    printf("%d\n", data.freq);
    printf("%d\n", data.band);
    printf("%s\n", data.module);
    printf("%d\n", data.aPID);
    printf("%d\n", data.vPID); 
    printf("%s\n", data.aType);    
    printf("%s\n", data.vType); 
    */
    
    // Test remote. 
    initRemote(callbackFunction);
    deinitRemote();
    
    /*
    // Test player. 
    initTunerPlayer(data.freq, data.band, DVB_T);
    playStream(data.aPID, audio);
    playStream(data.vPID, video);
    
    getchar();
    
    closeStream(video);
    closeStream(audio);
    deinitTunerPlayer();
    */
    
    return 0;
}
