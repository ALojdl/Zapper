#include <stdio.h>

#include "parser.h"
#include "tundem.h"
#include "remote.h"

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
    initRemote();
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
