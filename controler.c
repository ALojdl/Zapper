#include <stdio.h>
#include "controler.h"
#include "tundem.h"
#include "parser.h"

#define PAT_TABLE_ID            0x00
#define PAT_PID_NUM             0x00 
#define PMT_TABLE_ID            0x02
#define WAIT_FOR_NEW_PMT        100000 //us
#define LOOP_COUNT_MAX          10
#define AUDIO_TYPE_OF_STREAM    3
#define VIDEO_TYPE_OF_STREAM    2 
#define VOLUME_MIN              0
#define VOLUME_MAX              10
#define VOLUME_INIT_VAL         5
#define VOLUME_CONST            200000000

// playStream and closeStream are used in initHardware and deinitHardware.
t_Error playStream(uint32_t PID, stream_t type);
t_Error closeStream();

static char *path;
static uint16_t pmtVersionOnDisplay;
static init_data_t data;
static pat_table_t patTable;
static pmt_table_t pmtTable;
static uint8_t currentChannel;
static uint8_t minChannel;
static uint8_t maxChannel;
static uint8_t currentVolume;
static uint8_t volumeMuted;

static int32_t pmtFilterCallback (uint8_t *buffer);
static int32_t patFilterCallback (uint8_t *buffer);
static int32_t totFilterCallback (uint8_t *buffer);
static void changeChannel();

void savePath(char *pathToConfigFile);


void initHardware()
{    
    printf("\n====================\n  INICIJALIZACIJA\n====================\n");
    
    // Run stream defined in config file. 
    getConfiguration(path, &data);
    initTunerPlayer(data.freq, data.band, DVB_T);
    playStream(data.audioPID, audio);
    playStream(data.videoPID, video);    
    
    // Get data about channels on current frequency.
    currentChannel = 0; 
    minChannel = 1;
    initFilter(PAT_PID_NUM, PAT_TABLE_ID, patFilterCallback);
    
    // Set volume. 
    currentVolume = VOLUME_INIT_VAL;
    volumeMuted = 0;
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: Failed to set initial volume.\n");
    }
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
    // Apply round robin to channel.
    if (currentChannel > minChannel)
    {
        currentChannel --;
    }
    else
    {
        currentChannel = maxChannel;
    }
    
    // Request new PMT table.
    changeChannel();
}

void channelUp()
{
    // Apply round robin to channel.
    if (currentChannel < maxChannel)
    {
        currentChannel ++;
    }
    else
    {
        currentChannel = minChannel;
    }
    
    // Request new PMT table.
    changeChannel();
}

void goToChannel(uint16_t channel)
{
    // Check if channel is in the list. 
    if (channel >= minChannel && channel <= maxChannel)
    {
        currentChannel = channel;
        changeChannel();
    }
    else
    {
        printf("INFO: Channel %hu don't exist", channel);
        fflush(stdout);
    }
}

void volumeUp()
{
    // Check if muted.
    if (volumeMuted)
    {
        volumeMuted = 0;
    } 
    // If not, try to add volume.
    else if (currentVolume < VOLUME_MAX)
    {
        currentVolume ++;
    }
    
    // Apply new volume.
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: Failed to add volume.\n");
    }
    
    printf("INFO: Current volume %hu.\n", currentVolume);
}

void volumeDown()
{
    // Check if muted.
    if (volumeMuted)
    {
        volumeMuted = 0;
    } 
    // If not, try to sub volume.
    else if (currentVolume > VOLUME_MIN)
    {
        currentVolume --;
    }
    
    // Apply new volume.
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: Failed to sub volume.\n");
    }
    
    printf("INFO: Current volume %hu.\n", currentVolume);
}

void muteVolume()
{
    // Check if muted
    if (volumeMuted)
    {
        volumeMuted = 0;
        
        // Apply new volume.
        if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
        {
            printf("ERROR: Failed to unmute.\n");
        }
    }
    else
    {
        volumeMuted = 1;
        
        // Apply new volume.
        if (ERROR == volumeSet(VOLUME_MIN * VOLUME_CONST))
        {
            printf("ERROR: Failed to mute.\n");
        }
    }
    
    printf("INFO: M->Current volume %hu.\n", currentVolume);
}

void getInfo()
{
    printf("--------------------\n        INFO\n--------------------\n");
    if (currentChannel)
    {
        printPmtTable(&pmtTable);
    }
    else
    {
        printf("--------------------\n        NO INFO\n--------------------\n");
    }
}

void savePath(char *pathToConfigFile)
{
    path = pathToConfigFile;
    return;
}

static void changeChannel()
{
    if (currentChannel)
    {
        initFilter(patTable.patServiceInfoArray[currentChannel - 1].PID,
            PMT_TABLE_ID, pmtFilterCallback);
    }
    else
    {
        printf("WARNING: Can't get PMT table for current channel.\n");
    }
    return;
}

static void getFirstVideoAndAudio()
{
    uint8_t i;
    
    data.audioPID = 0;
    data.videoPID = 0;
    
    for (i = 0; i < pmtTable.serviceInfoCount; i ++)
    {
        if (pmtTable.pmtServiceInfoArray[i].streamType == VIDEO_TYPE_OF_STREAM)
        {
            data.videoPID = pmtTable.pmtServiceInfoArray[i].PID;
        }
        else if (pmtTable.pmtServiceInfoArray[i].streamType 
            == AUDIO_TYPE_OF_STREAM)
        {
            data.audioPID = pmtTable.pmtServiceInfoArray[i].PID;
        }
        
        // If we have discovered audio and video PID then quit.
        if (data.audioPID != 0 && data.videoPID != 0)
        {
            break;
        }
    }
    
    return;    
}

static int32_t patFilterCallback (uint8_t *buffer)
{
    if (ERROR == parsePatTable(buffer, &patTable))
    {
        printf("ERROR: %s crashed while parsing table!");
        deinitFilter(patFilterCallback);
    }
    else
    {        
        printPatTable(&patTable);
        deinitFilter(patFilterCallback);
        maxChannel = patTable.serviceInfoCount;
    }
    
    return 0;
}

static int32_t pmtFilterCallback (uint8_t *buffer)
{
    if (ERROR == parsePmtTable(buffer, &pmtTable))
    {
        printf("ERROR: %s crashed while parsing table!");
        deinitFilter(pmtFilterCallback);
    }
    else
    {        
        // Change channel.
        getFirstVideoAndAudio();
        
        // Close current streams. 
        closeStream(video);
        closeStream(audio);
        
        // Open streams.
        playStream(data.audioPID, audio);    
        if (data.videoPID)
        {
            playStream(data.videoPID, video);
        }
        
        deinitFilter(pmtFilterCallback);
    }
    
    return 0;
}

static int32_t totFilterCallback (uint8_t *buffer)
{
    return 0;
}
