#include <stdio.h>
#include "middleware.h"
#include "tundem.h"
#include "parser.h"

#define PAT_TABLE_ID            0x00
#define PAT_PID_NUM             0x00 
#define PMT_TABLE_ID            0x02
#define TOT_PID_NUM             0x14
#define TOT_TABLE_ID            0x73
#define WAIT_FOR_NEW_PMT        100000 //us
#define LOOP_COUNT_MAX          10
#define AUDIO_TYPE_OF_STREAM    3
#define VIDEO_TYPE_OF_STREAM    2 
#define VOLUME_MIN              0
#define VOLUME_MAX              100
#define VOLUME_INIT_VAL         50
#define VOLUME_CONST            20000000


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
static uint8_t videoStreaming;
static char dmyTime[12];
static char parseTime[12];
static changed_channel_callback_t channelCallback;

static int32_t pmtFilterCallback (uint8_t *buffer);
static int32_t patFilterCallback (uint8_t *buffer);
static int32_t totFilterCallback (uint8_t *buffer);
static void changeChannel();

void savePath(char *pathToConfigFile);


void initHardware()
{                
    // Play stream defined in config file. 
    getConfiguration(path, &data);
    initTunerPlayer(data.freq, data.band, data.module);
    playStream(data.audioPID, audio, data.audioType);
    playStream(data.videoPID, video, data.videoType);  
    videoStreaming = 1;  
    
    // Get data about channels on current frequency.
    currentChannel = 0; 
    minChannel = 1;
    initFilter(PAT_PID_NUM, PAT_TABLE_ID, patFilterCallback);
    sprintf(dmyTime, "10/10/2010");
    
    // Set volume. 
    currentVolume = VOLUME_INIT_VAL;
    volumeMuted = 0;
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: %s failed to set initial volume.\n", __func__);
    }
    
    // Init graphics.
    initGraphic();    
}

void deinitHardware()
{    
    // Close streams.
    if (videoStreaming)
    {
        closeStream(video);
        videoStreaming = 0;
    }
    closeStream(audio);
    
    // Deinit tuner, player and graphics.
    deinitTunerPlayer();
    deinitGraphic();
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
    
    // Request new PMT table, indirectly changing the channel.
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
    
    // Request new PMT table, indirectly changing the channel.
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
    // If there's no channel, just inform user.
    else
    {
#ifdef DEBUG_INFO
        printf("INFO: Channel %hu don't exist", channel);
#endif
    }
}

void volumeUp(uint8_t volumeStep)
{
    // Check if muted.
    if (volumeMuted)
    {
        volumeMuted = 0;
    } 
    // If not, try to add volume.
    else if (currentVolume < VOLUME_MAX)
    {
        currentVolume += volumeStep;
    }
    
    // Apply new volume.
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: Failed to add volume.\n");
    }
}

void volumeDown(uint8_t volumeStep)
{
    // Check if muted.
    if (volumeMuted)
    {
        volumeMuted = 0;
    } 
    // If not, try to sub volume.
    else if (currentVolume > VOLUME_MIN)
    {
        currentVolume -= volumeStep;
    }
    
    // Apply new volume.
    if (ERROR == volumeSet(currentVolume * VOLUME_CONST))
    {
        printf("ERROR: Failed to sub volume.\n");
    }
}

void muteVolume()
{
    // Check if muted.
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
}

void getInfo()
{  
    if (currentChannel)
    {
       drawInfoBar(currentChannel, pmtTable.teletextExist, dmyTime);
    }
    else
    {
#ifdef DEBUG_INFO
        printf("INFO: No information for current channel.\n");
#endif
    }
}

void savePath(char *pathToConfigFile)
{
    path = pathToConfigFile;
}

static void changeChannel()
{
    // Check if there is PMT table for current channel.
    if (currentChannel)
    {
        initFilter(patTable.patServiceInfoArray[currentChannel - 1].PID,
            PMT_TABLE_ID, pmtFilterCallback);
    }
    else
    {
#ifdef DEBUG_INFO
        printf("INFO: Can't get PMT table for current channel.\n");
#endif
    }
    return;
}

static void getFirstVideoAndAudio()
{
    uint8_t i;
    
    data.audioPID = 0;
    data.videoPID = 0;
    
    // Going trough services whole service list.
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
    // Check if there was error while parsing.
    if (ERROR == parsePatTable(buffer, &patTable))
    {
        printf("ERROR: %s crashed while parsing table!");
        deinitFilter(patFilterCallback);
        return -1;
    }
    else
    {        
        deinitFilter(patFilterCallback);
        maxChannel = patTable.serviceInfoCount;
        return 0;
    }
}

static int32_t pmtFilterCallback (uint8_t *buffer)
{
    // Check if there was error while parsing.
    if (ERROR == parsePmtTable(buffer, &pmtTable))
    {
        printf("ERROR: %s crashed while parsing table!");
        deinitFilter(pmtFilterCallback);
        return -1;
    }
    else
    {   
        // Change channel.     
        getFirstVideoAndAudio();
  
        if (videoStreaming)
        {
            closeStream(video);
            videoStreaming = 0;
        }
        closeStream(audio);

        playStream(data.audioPID, audio, AUDIO_TYPE_MPEG_AUDIO);    
        if (data.videoPID)
        {
            playStream(data.videoPID, video, VIDEO_TYPE_MPEG2); 
            videoStreaming = 1;
        }
        
        deinitFilter(pmtFilterCallback);
        channelCallback(currentChannel);
        return 0;
    }
}

static int32_t totFilterCallback (uint8_t *buffer)
{
    // Check if there was error while parsing.
    if (ERROR == parseTotTable(buffer, dmyTime))
    {
        printf("ERROR: Error while parsing TOT.\n");
        deinitFilter(totFilterCallback);
        return -1;
    }
    else
    {
        deinitFilter(totFilterCallback);
        return 0;
    }
}

void registerChannelCallback(changed_channel_callback_t callbackFunction)
{
    channelCallback = callbackFunction;
}

uint8_t getVolume()
{
    if (volumeMuted)
    {
        return VOLUME_MIN;
    }
    else
    {
        return currentVolume;
    }
}

channel_info_t getChannelInfo()
{
    uint8_t i;
    channel_info_t channelInfo;
    channelInfo.channelNumber = currentChannel;
    channelInfo.channelIndex = currentChannel;
    channelInfo.teletextExist = pmtTable.teletextExist;
    channelInfo.audioNumber = 0;
    
    for (i = 0; i < pmtTable.serviceInfoCount; i++)
    {
        if (pmtTable.pmtServiceInfoArray[i].streamType 
            == AUDIO_TYPE_OF_STREAM)
        {
            channelInfo.audioNumber ++;
        }
    }
    
    return channelInfo;
}
