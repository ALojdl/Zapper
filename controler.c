#include <stdio.h>
#include "controler.h"
#include "tundem.h"
#include "parser.h"

t_Error initTunerPlayer(uint32_t freq, uint32_t band, t_Module module);
t_Error deinitTunerPlayer();
t_Error playStream(uint32_t PID, stream_t type);
t_Error closeStream();

static pat_table_t patTable;
static int32_t filterCallback (uint8_t *buffer);

void initHardware()
{
    init_data_t data;
    
    printf("\n====================\n  INICIJALIZACIJA\n====================\n");
    
    // Run stream defined in config file. 
    getConfiguration("./config.ini", &data);
    initTunerPlayer(data.freq, data.band, DVB_T);
    playStream(data.aPID, audio);
    playStream(data.vPID, video);    
    
    // Get data about channels on current frequency.
    
}

void deinitHardware()
{
    printf("\n====================\n DEINICIJALIZACIJA\n====================\n");
    closeStream(video);
    closeStream(audio);
    deinitTunerPlayer();
}

void channelDown()
{
    printf("CH-\n");
}

void channelUp()
{
    printf("CH+\n");
}

void goToChannel(uint16_t channel)
{
    printf("Go to channel: %hu\n", channel);
}

void volumeUp()
{
    printf("V+\n");
}

void volumeDown()
{
    printf("V-\n");
}

void muteVolume()
{
    printf("MUTE\n");
}

void getInfo()
{
    printf("--------------------\n        INFO\n--------------------\n");
    initFilter(0x0, 0x0, filterCallback);
}

int32_t filterCallback (uint8_t *buffer)
{
    if (ERROR == parsePatTable(buffer, &patTable))
    {
        printf("ERROR: %s crashed while parsing table!");
    }
    else
    {        
        printPatTable(&patTable);
        deinitFilter(filterCallback);
    }
    
    return 0;
}
