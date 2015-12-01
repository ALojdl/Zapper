#include <stdio.h>
#include <string.h>

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

t_Error parseTotTable(const uint8_t *totBuffer, char *date)
{
    uint8_t tableId;
    uint16_t mjdTime;
    uint16_t tmpYear;
    uint16_t tmpMonth;
    uint16_t day;
    uint16_t month;
    uint16_t year;    
    uint16_t K;
    
    tableId = totBuffer[0];
   
    if (tableId != TOT_TABLE_ID)
    {
        printf("ERROR: %s fetched no TOT table.\n", __func__);
        return ERROR;
    }
    
    mjdTime = totBuffer[3];
    mjdTime = mjdTime << 8;
    mjdTime += totBuffer[4];
    
    printf("INFO: mjd: %hu\n", mjdTime);
    
    tmpYear = (int) ((mjdTime - 15078.2) / 365.25);
    tmpMonth = (int) ((mjdTime - 14956.1 - (int) (tmpYear * 365.25)) 
        / 30.6001);
    day = mjdTime - 14956 - (int) (tmpYear * 365.25) 
        - (int) (tmpMonth * 30.6001);
        
    if (tmpMonth == 14 || tmpMonth == 15)
    {
        K = 1;
    }
    else
    {
        K = 0;
    }
    year = 1900 + tmpYear + K;
    month = tmpMonth - 1 - K * 12;
    
    sprintf(date, "%hu/%hu/%hu", month, day, year);  
     
    
    return NO_ERROR;
}
