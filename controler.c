#include <stdio.h>
#include "controler.h"
#include "tundem.h"
#include "parser.h"

#define PAT_TABLE_ID    0x00
#define PAT_PID_NUM     0x00 
#define PMT_TABLE_ID    0x02

// playStream and closeStream are used in initHardware and deinitHardware.
t_Error playStream(uint32_t PID, stream_t type);
t_Error closeStream();

static char *path;
static pat_table_t patTable;
static pmt_table_t pmtTable;

static int32_t pmtFilterCallback (uint8_t *buffer);
static int32_t patFilterCallback (uint8_t *buffer);
static int32_t totFilterCallback (uint8_t *buffer);

static uint8_t currentChannel;
static uint8_t minChannel;
static uint8_t maxChannel;

void savePath(char *pathToConfigFile)
{
    path = pathToConfigFile;
    return;
}

void initHardware()
{
    init_data_t data;
    
    printf("\n====================\n  INICIJALIZACIJA\n====================\n");
    
    // Run stream defined in config file. 
    getConfiguration(path, &data);
    initTunerPlayer(data.freq, data.band, DVB_T);
    playStream(data.aPID, audio);
    playStream(data.vPID, video);    
    
    // Get data about channels on current frequency.
    currentChannel = 0; 
    minChannel = 1;
    initFilter(PAT_PID_NUM, PAT_TABLE_ID, patFilterCallback);
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
    if (currentChannel > minChannel)
    {
        currentChannel --;
        getInfo();
    }
    else
    {
        currentChannel = maxChannel;
        getInfo();
    }
}

void channelUp()
{
     if (currentChannel < maxChannel)
    {
        currentChannel ++;
        getInfo();
    }
    else
    {
        currentChannel = minChannel;
        getInfo();
    }
}

void goToChannel(uint16_t channel)
{
    if (channel >= minChannel && channel <= maxChannel)
    {
        currentChannel = channel;
        getInfo();
    }
    else
    {
        printf("INFO: Channel %hu don't exist", channel);
        fflush(stdout);
    }
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
    if (currentChannel)
    {
        initFilter(patTable.patServiceInfoArray[currentChannel - 1].PID,
            PMT_TABLE_ID, pmtFilterCallback);
        printf("    PID: %d\n",
            patTable.patServiceInfoArray[currentChannel - 1].PID);
        printf("     CH: %d\n", currentChannel);
    }
    else
    {
        printf("--------------------\n        NO INFO\n--------------------\n");
    }
}

int32_t patFilterCallback (uint8_t *buffer)
{
    if (ERROR == parsePatTable(buffer, &patTable))
    {
        printf("ERROR: %s crashed while parsing table!");
    }
    else
    {        
        printPatTable(&patTable);
        deinitFilter(patFilterCallback);
        maxChannel = patTable.serviceInfoCount;
    }
    
    return 0;
}

int32_t pmtFilterCallback (uint8_t *buffer)
{
    if (ERROR == parsePmtTable(buffer, &pmtTable))
    {
        printf("ERROR: %s crashed while parsing table!");
    }
    else
    {        
        printPmtTable(&pmtTable);
        deinitFilter(pmtFilterCallback);
    }
    
    return 0;
}

int32_t totFilterCallback (uint8_t *buffer)
{
    return 0;
}
