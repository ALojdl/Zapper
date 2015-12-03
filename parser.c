#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "parser.h"

#define MAX_CHARACTERS          20
#define PAT_INFO_SIZE           4
#define TELETEXT_TAG            0x56
#define PAT_TABLE_ID            0x00
#define TOT_TABLE_ID            0x73
#define DVB_T                   0
#define DVB_T2                  1

static const char moduleT[] = "DVB-T";
static const char moduleT2[] = "DVB-T2";
static const char AC3[] = "AC3";
static const char MPEG[] = "MPEG";
static const char MPEG2[] = "MPEG2";
static const char MPEG4[] = "MPEG4";


void getConfiguration(const char *path, init_data_t *data)
{
    FILE *configFile;
    char tmp[MAX_CHARACTERS];
    char *pch;
    
    configFile = fopen(path, "r");
    if (configFile == NULL) 
    {
        printf("%s\t: ERROR -> Error opening file!", __func__);
        fflush(stdout);
        return;
    }

    //  [dvb-t]      
    fscanf(configFile, "%s", tmp);
    
    //  frequency=470
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%d", &data->freq);
    
    //  bandwith=8
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp, "=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%d", &data->band);
    
    //  module=DVB-T
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%s", tmp);
    
    // Make a mapping.
    if (!strcmp(tmp, moduleT))
    {
        data->module = DVB_T;
    }
    else if(!strcmp(tmp, moduleT2))
    {
        data->module = DVB_T2;
    }
    else 
    {
        printf("ERROR: %s failed while reading module info.\n", __func__);
    }
    
    //  [initial_channel]
    fscanf(configFile, "%s", tmp); 
    
    //  aPID=101
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%d", &data->audioPID);   
    
    //  vPID=102
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%d", &data->videoPID);  
    
    //  aType=ac3
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%s", tmp);
    
    // Make a mapping.
    if (!strcmp(tmp, AC3))
    {
        data->audioType = AUDIO_TYPE_DOLBY_AC3;
    }
    else if(!strcmp(tmp, MPEG))
    {
        data->audioType = AUDIO_TYPE_MPEG_AUDIO;
    }
    else 
    {
        printf("ERROR: %s failed while reading audio info.\n", __func__);
    }
    
    //  vType=mpeg2
    fscanf(configFile, "%s", tmp);    
    pch = strtok(tmp,"=");
    pch = strtok(NULL, "=");
    sscanf(pch, "%s", tmp); 
    
    // Make a mapping.
    if (!strcmp(tmp, MPEG2))
    {
        data->videoType = VIDEO_TYPE_MPEG2;
    }
    else if(!strcmp(tmp, MPEG4))
    {
        data->videoType = VIDEO_TYPE_MPEG4;
    }
    else 
    {
        printf("ERROR: %s failed while reading video info.\n", __func__);
    }
    
    fclose(configFile);
}    

t_Error parsePatHeader(const uint8_t *patHeaderBuffer, pat_header_t *patHeader)
{    
    uint16_t tmp;
    
    if (patHeaderBuffer == NULL || patHeader == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }

    patHeader->tableId = patHeaderBuffer[0]; 
    if (patHeader->tableId != PAT_TABLE_ID)
    {
        printf("ERROR: %s it is not a PAT Table.\n", __func__);
        return ERROR;
    }
    
    // Get PAT section length from buffer.
    tmp = patHeaderBuffer[1];
    tmp = tmp << 8;
    tmp = tmp + patHeaderBuffer[2];
    tmp = tmp & 0x0FFF;    
    patHeader->sectionLength = tmp;
    
    // Get PAT version number from buffer.
    tmp = patHeaderBuffer[5];
    tmp = tmp >> 1;
    tmp = tmp & 0x1F;
    patHeader->versionNumber = tmp;

    return NO_ERROR;
}

t_Error parsePatServiceInfo(const uint8_t *patServiceInfoBuffer,
    pat_service_info_t *patServiceInfo)
{
    uint16_t tmp;
    
    if (patServiceInfoBuffer == NULL || patServiceInfo == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }
    
    // Get PAT program number.
    tmp = patServiceInfoBuffer[0];
    tmp = tmp << 8;
    tmp = tmp + patServiceInfoBuffer[1];
    tmp = tmp & 0xFFFF;
    patServiceInfo->programNumber = tmp;
    
    // Get PAT PID. 
    tmp = patServiceInfoBuffer[2];
    tmp = tmp << 8;
    tmp = tmp + patServiceInfoBuffer[3];
    tmp = tmp & 0x1FFF;
    patServiceInfo->PID = tmp;
    
    return NO_ERROR;
}

t_Error parsePatTable(const uint8_t *patSectionBuffer, pat_table_t *patTable)
{
    uint8_t *currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if (patSectionBuffer == NULL || patTable == NULL)
    {
        printf("ERROR: %s received parameters are not ok\n", __func__);
        return ERROR;
    }
    
    if (parsePatHeader(patSectionBuffer, &(patTable->patHeader)) != NO_ERROR)
    {
        printf("ERROR: %s parsing PAT header.\n", __func__);
        return ERROR;
    }
    
    parsedLength = 9; 
    currentBufferPosition = (uint8_t *)&patSectionBuffer[8]; 
    patTable->serviceInfoCount = 0; 
    
    while (parsedLength < patTable->patHeader.sectionLength)
    {
        if (patTable->serviceInfoCount > (MAX_NUMBER_OF_PIDS_IN_PAT - 1))
        {
            printf("ERROR: %s there is not enough space for Service info\n",
                __func__);
            return ERROR;
        }
        
        if (parsePatServiceInfo(currentBufferPosition, 
            &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) 
                == NO_ERROR)
        {
            currentBufferPosition += PAT_INFO_SIZE; 
            parsedLength += PAT_INFO_SIZE; 
                        
            // Check if this is real channel, it is not a zero.
            if ( (patTable->patServiceInfoArray[patTable->serviceInfoCount])
                .programNumber )
            {
                patTable->serviceInfoCount ++;
            }
        }    
    }
    
    return NO_ERROR;
}

t_Error printPatTable(pat_table_t *patTable)
{
    uint8_t i = 0;
    
    if (patTable == NULL)
    {
        printf("ERROR: %s received parameter is not ok.\n", __func__);
        return ERROR;
    }
    
    printf("\n------------ PAT TABLE SECTION ------------------\n");
    printf("table_id         |  %d\n", patTable->patHeader.tableId);
    printf("section_length   |  %d\n", patTable->patHeader.sectionLength);
    printf("table_version    |  %d\n", patTable->patHeader.versionNumber);
    
    for (i = 0; i < patTable->serviceInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number   |  %d\n",
            patTable->patServiceInfoArray[i].programNumber);
        printf("pid              |  %d\n", 
            patTable->patServiceInfoArray[i].PID); 
    }
    printf("\n============= PAT TABLE SECTION =================\n");
    
    return NO_ERROR;
}

uint8_t findTeletext(const uint8_t* infoSectionBuffer, uint16_t infoSectionLength)
{
    uint8_t descriptionTag;
    uint8_t descriptionLength;
    uint16_t parsedLength = 0;
    
    while (parsedLength < infoSectionLength)
    {
        descriptionTag = infoSectionBuffer[parsedLength];
        descriptionLength = infoSectionBuffer[parsedLength + 1];
        
        if (descriptionTag == TELETEXT_TAG)
        {
            return 1;
        }
        else
        {
            parsedLength = parsedLength + descriptionLength + 2;
            //printf("DEBUG: parsLength %hu\n", parsedLength);
        }
    }
    
    return 0;
}

t_Error parsePmtHeader(const uint8_t *pmtHeaderBuffer, pmt_header_t *pmtHeader)
{    
    uint16_t tmp;
    
    if (pmtHeaderBuffer == NULL || pmtHeader == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }

    pmtHeader->tableId = pmtHeaderBuffer[0]; 
    pmtHeader->programNumber = pmtHeaderBuffer[3];
    
    // Get PMT section length from buffer.
    tmp = pmtHeaderBuffer[1];
    tmp = tmp << 8;
    tmp = tmp + pmtHeaderBuffer[2];
    tmp = tmp & 0x0FFF;
	pmtHeader->sectionLength = tmp;
    
    // Get PMT version number from buffer.
    tmp = pmtHeaderBuffer[5];
    tmp = tmp >> 1;
    tmp = tmp & 0x1F;
    pmtHeader->versionNumber = tmp;

    return NO_ERROR;
}

t_Error parsePmtServiceInfo(const uint8_t *pmtServiceInfoBuffer,
    pmt_stream_t *pmtStreamInfo)
{
    uint16_t tmp;
    
    if (pmtServiceInfoBuffer == NULL || pmtStreamInfo == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }
    
    // Get PMT stream type.
    pmtStreamInfo->streamType = pmtServiceInfoBuffer[0];
    
    // Get PMT PID. 
    tmp = pmtServiceInfoBuffer[1];
    tmp = tmp << 8;
    tmp = tmp + pmtServiceInfoBuffer[2];
    tmp = tmp & 0x1FFF;
    pmtStreamInfo->PID = tmp;
    
    return NO_ERROR;
}

t_Error parsePmtTable(const uint8_t *pmtBuffer, pmt_table_t *pmtTable)
{
    uint8_t *currentBufferPosition = NULL;
    uint16_t sectionLength;
    uint16_t infoLength;
    uint16_t index;
    
    if (pmtBuffer == NULL || pmtTable == NULL)
    {
        printf("ERROR: %s received parameters are not ok\n", __func__);
        return ERROR;
    }
    
    if (parsePmtHeader(pmtBuffer, &(pmtTable->pmtHeader)) != NO_ERROR)
    {
        printf("ERROR: %s parsing PMT header.\n", __func__);
        return ERROR;
    }
    
    sectionLength = pmtTable->pmtHeader.sectionLength;
    index = 10;		
    
    // Get length of ES info section.
	infoLength = pmtBuffer[index];
	infoLength = infoLength << 8;
	infoLength = infoLength + pmtBuffer[index + 1];
	infoLength = infoLength & 0x0FFF;
	
	index = index + 2 + infoLength; 
	sectionLength -= 13;
	pmtTable->serviceInfoCount = 0;
	pmtTable->teletextExist = 0;
	
	while (sectionLength)
	{
	    currentBufferPosition = (uint8_t *)&pmtBuffer[index];
	    if (pmtTable->serviceInfoCount > (MAX_NUMBER_OF_STREAMS_IN_PMT - 1))
        {
            printf("ERROR: %s there is not enough space for Stream info\n",
                __func__);
            return ERROR;
        }
        
        if (parsePmtServiceInfo(currentBufferPosition, 
                &(pmtTable->pmtServiceInfoArray[pmtTable->serviceInfoCount]))
                == NO_ERROR)
        {
            // Get info length.
            infoLength = pmtBuffer[index + 3];
			infoLength = infoLength << 8;
			infoLength += pmtBuffer[index + 4];
			infoLength = infoLength & 0x0FFF;
			
			// Check if there is a teletext.
			if (!pmtTable->teletextExist)
			{
			    pmtTable->teletextExist = findTeletext
			        ((currentBufferPosition + 5), infoLength);
			}
			
			// Move pointer to new location based on info length.
            index = index + 5 + infoLength;
			sectionLength -= (5 + infoLength);
            pmtTable->serviceInfoCount ++;
        }    
	}	
    
    return NO_ERROR;
}

t_Error printPmtTable(pmt_table_t *pmtTable)
{
    uint8_t i = 0;
    
    if (pmtTable == NULL)
    {
        printf("ERROR: %s received parameter is not ok.\n", __func__);
        return ERROR;
    }
    
    printf("\n------------ PMT TABLE SECTION ------------------\n");
    printf("table_id         |  %d\n", pmtTable->pmtHeader.tableId);
    printf("section_length   |  %d\n", pmtTable->pmtHeader.sectionLength);
    printf("program_number   |  %d\n", pmtTable->pmtHeader.programNumber);
    printf("table_version    |  %d\n", pmtTable->pmtHeader.versionNumber);
    printf("teletext_exist   |  %d\n", pmtTable->teletextExist);
    
    for (i=0; i<pmtTable->serviceInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("stream_type      |  %d\n",
            pmtTable->pmtServiceInfoArray[i].streamType);
        printf("pid              |  %d\n",
            pmtTable->pmtServiceInfoArray[i].PID); 
    }
    printf("\n============= PMT TABLE SECTION =================\n");
    
    return NO_ERROR;
}

void findLocalOffset(const uint8_t* buffer, uint16_t bufferLength,
    uint8_t *localTimePolarity, uint16_t *localTimeOffset)
{
    uint16_t parsedLength = 0;
    uint8_t descriptorTag;
    uint8_t descriptorLength;
    uint8_t countryCode[3];
    
    while (parsedLength < bufferLength)
    {
        descriptorTag = buffer[parsedLength];
        descriptorLength = buffer[parsedLength + 1];
 
        
        if (descriptorTag == 0x58)
        {
            countryCode[0] = buffer[parsedLength + 2];
            countryCode[1] = buffer[parsedLength + 3];
            countryCode[2] = buffer[parsedLength + 4];
            
            if (countryCode[0] == 'E' && countryCode[1] == 'S' && 
                countryCode[2] == 'P')
            {
                *localTimePolarity = buffer[parsedLength + 5];
                *localTimePolarity = *localTimePolarity & 0x01;
                *localTimeOffset = buffer[parsedLength + 6];
                *localTimeOffset = *localTimeOffset << 8;
                *localTimeOffset += buffer[parsedLength + 7];

                return; 
            }
        }
        parsedLength += descriptorLength + 2;  
    }
}

void convertTimeToStreamTime(uint16_t mjdTime, 
    uint8_t *UTCTime, stream_time_t *myTime)
{
    uint16_t tmpYear;
    uint16_t tmpMonth;
    uint16_t K;
    
    uint8_t hours;
    uint8_t tmpMinutes;
    uint8_t tmpSeconds;
    
    tmpYear = (int) ((mjdTime - 15078.2) / 365.25);
    tmpMonth = (int) ((mjdTime - 14956.1 - (int) (tmpYear * 365.25)) 
        / 30.6001);
        
    if (tmpMonth == 14 || tmpMonth == 15)
    {
        K = 1;
    }
    else
    {
        K = 0;
    }
    myTime->year = tmpYear + K;
    myTime->month = tmpMonth - 1 - K * 12;
    myTime->day = mjdTime - 14956 - (int) (tmpYear * 365.25) 
        - (int) (tmpMonth * 30.6001);
    
    myTime->hours = (UTCTime[0] >> 4) * 10 + (UTCTime[0] & 0x0F);
    myTime->minutes = (UTCTime[1] >> 4) * 10 + (UTCTime[1] & 0x0F);
    myTime->seconds = (UTCTime[2] >> 4) * 10 + (UTCTime[2] & 0x0F);
    
    printf("INFO: Time read from stream: %hu/%hu/%hu %hu:%hu:%hu .\n", 
        myTime->year, myTime->month, myTime->day, myTime->hours, myTime->minutes, 
        myTime->seconds);
}

int16_t convertOffsetToSeconds(uint8_t localTimePolarity, uint16_t localTimeOffset)
{    
    uint16_t tmpHours;
    uint16_t tmpMinutes;
    int16_t seconds;
    
    tmpHours = (localTimeOffset >> 12) * 10 + ((localTimeOffset >> 8) & 0x0F);
    tmpMinutes = ((localTimeOffset >> 4) & 0x00F) * 10 + (localTimeOffset & 0x000F);
    
    if (localTimePolarity)
    {
        seconds = -(tmpHours * 3600 + tmpMinutes * 60);
    }
    else
    {
        seconds = (tmpHours * 3600 + tmpMinutes * 60);
    }

#ifdef DEBUG_INFO    
    printf("INFO: Offset in seconds %d.\n", seconds);
#endif
    
    return seconds;
}

void addOffset(stream_time_t *myTime, uint16_t seconds)
{
    struct tm time;
    struct tm *newTime;
    time_t  epochTime;    
    
    // Copy values
    time.tm_sec = myTime->seconds;
    time.tm_min = myTime->minutes;
    time.tm_hour = myTime->hours;
    time.tm_mday = myTime->day;
    time.tm_mon = myTime->month;
    time.tm_year = myTime->year;
    time.tm_isdst = 0;

    // Convert to seconds, add offset and convert again.
    epochTime = mktime(&time);
    epochTime += seconds;
    newTime = gmtime(&epochTime);
    
    // Copy values
    myTime->seconds = newTime->tm_sec;
    myTime->minutes = newTime->tm_min;
    myTime->hours = newTime->tm_hour;
    myTime->day = newTime->tm_mday;
    myTime->month = newTime->tm_mon;
    myTime->year = newTime->tm_year;   

#ifdef DEBUG_INFO    
    printf("INFO: Time calculated from stream: %hu/%hu/%hu %hu:%hu:%hu\n",
        myTime->year, myTime->month, myTime->day, myTime->hours, 
        myTime->minutes, myTime->seconds);
#endif
}

t_Error parseTotTable(const uint8_t *totBuffer, stream_time_t *streamTime)
{
    uint16_t mjdTime;  
    uint16_t loopLength;
    uint16_t localTimeOffset;
    uint16_t parsedLength = 0;
    uint16_t seconds;
    uint8_t tableId = totBuffer[0];
    uint8_t localTimePolarity;    
    uint8_t UTCTime[3];
   
    if (tableId != TOT_TABLE_ID)
    {
        printf("ERROR: %s fetched no TOT table.\n", __func__);
        return ERROR;
    }
    
    // Parse day, month and year.
    mjdTime = totBuffer[3];
    mjdTime = mjdTime << 8;
    mjdTime += totBuffer[4];  
    
    // Parse UTC time.
    UTCTime[0] = totBuffer[5]; 
    UTCTime[1] = totBuffer[6]; 
    UTCTime[2] = totBuffer[7]; 
    
    // Parse length of descriptor part. 
    loopLength = totBuffer[8];
    loopLength = loopLength & 0x0F;
    loopLength = loopLength << 8;
    loopLength += totBuffer[9];
    
    findLocalOffset(&totBuffer[10], loopLength, 
        &localTimePolarity, &localTimeOffset);
        
#ifdef DEBUG_INFO    
    printf("INFO: -------------------------------------------------------- \n");
    printf("INFO: mjd = %hu\n", mjdTime);
    printf("INFO: polarity = %hu\n", localTimePolarity);
    printf("INFO: offset = %x\n", localTimeOffset);
    printf("INFO: UTCtime = %x%x%x\n", UTCTime[0], UTCTime[1], UTCTime[2]);
#endif
    
    convertTimeToStreamTime(mjdTime, UTCTime, streamTime);    
    seconds = convertOffsetToSeconds(localTimePolarity, localTimeOffset);
    addOffset(streamTime, seconds);    
    
    return NO_ERROR;
}
