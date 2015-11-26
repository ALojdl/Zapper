#include <stdio.h>
#include <string.h>

#include "parser.h"

#define TELETEXT_TAG 0x56

void getConfiguration(const char *path, init_data_t *data)
{
    FILE *config_file;
    char tmp[20];
    char * pch;
    
    config_file = fopen(path, "r");
    if (config_file == NULL) 
    {
        printf("%s\t: ERROR -> Error opening file!", __func__);
        fflush(stdout);
        return;
    }

    //  [dvb-t]      
    fscanf(config_file, "%s", tmp);
    
    //  frequency=470
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->freq);
    
    //  bandwith=8
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->band);
    
    //  module=DVB-T
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->module);
    
    //  [initial_channel]
    fscanf(config_file, "%s", tmp); 
    
    //  aPID=101
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->aPID);   
    
    //  vPID=102
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->vPID);  
    
    //  aType=ac3
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->aType);
    
    //  vType=mpeg2
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->vType); 
    
    fclose(config_file);
}    

t_Error parsePatHeader(const uint8_t* patHeaderBuffer, pat_header_t* patHeader)
{    
    uint16_t tmp;
    
    if (patHeaderBuffer == NULL || patHeader == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }

    patHeader->tableId = (uint8_t)* patHeaderBuffer; 
    if (patHeader->tableId != 0x00)
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

t_Error parsePatServiceInfo(const uint8_t* patServiceInfoBuffer, pat_service_info_t* patServiceInfo)
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

t_Error parsePatTable(const uint8_t* patSectionBuffer, pat_table_t* patTable)
{
    uint8_t * currentBufferPosition = NULL;
    uint32_t parsedLength = 0;
    
    if (patSectionBuffer == NULL || patTable == NULL)
    {
        printf("ERROR: %s received parameters are not ok\n", __func__);
        return ERROR;
    }
    
    if (parsePatHeader(patSectionBuffer,&(patTable->patHeader)) != NO_ERROR)
    {
        printf("ERROR: %s parsing PAT header.\n", __func__);
        return ERROR;
    }
    
    parsedLength = 12 /*PAT header size*/ - 3 /*Not in section length*/;
    currentBufferPosition = (uint8_t *)(patSectionBuffer + 8); /* Position after last_section_number */
    patTable->serviceInfoCount = 0; /* Number of services info presented in PAT table */
    
    while (parsedLength < patTable->patHeader.sectionLength)
    {
        if (patTable->serviceInfoCount > MAX_NUMBER_OF_PIDS_IN_PAT - 1)
        {
            printf("ERROR: %s there is not enough space for Service info\n", __func__);
            return ERROR;
        }
        
        if (parsePatServiceInfo(currentBufferPosition, 
            &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) == NO_ERROR)
        {
            currentBufferPosition += 4; /* Size from program_number to pid */
            parsedLength += 4; /* Size from program_number to pid */
            
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

t_Error printPatTable(pat_table_t* patTable)
{
    uint8_t i = 0;
    
    if (patTable == NULL)
    {
        printf("ERROR: %s received parameter is not ok.\n", __func__);
        return ERROR;
    }
    
    printf("\n------------ PAT TABLE SECTION ------------------\n");
    printf("table_id         |  %d\n",patTable->patHeader.tableId);
    printf("section_length   |  %d\n",patTable->patHeader.sectionLength);
    printf("table_version    |  %d\n",patTable->patHeader.versionNumber);
    
    for (i=0; i<patTable->serviceInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number   |  %d\n",patTable->patServiceInfoArray[i].programNumber);
        printf("pid              |  %d\n",patTable->patServiceInfoArray[i].PID); 
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
            return 0x1;
        }
        else
        {
            parsedLength = parsedLength + descriptionLength + 2;
            printf("DEBUG: parsLength %hu\n", parsedLength);
        }
    }
    
    return 0x0;
}

t_Error parsePmtHeader(const uint8_t* pmtHeaderBuffer, pmt_header_t* pmtHeader)
{    
    uint16_t tmp;
    
    if (pmtHeaderBuffer == NULL || pmtHeader == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }

    pmtHeader->tableId = (uint8_t)* pmtHeaderBuffer; 
    pmtHeader->programNumber = (uint16_t)* (pmtHeaderBuffer + 3);
    
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

t_Error parsePmtServiceInfo(const uint8_t* pmtServiceInfoBuffer, pmt_stream_t* pmtStreamInfo)
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

t_Error parsePmtTable(const uint8_t* pmtBuffer, pmt_table_t* pmtTable)
{
    uint8_t * currentBufferPosition = NULL;
    uint16_t sec_len;
    uint16_t info_length;
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
    
    sec_len = pmtTable->pmtHeader.sectionLength;
    index = 10;		
    
	info_length = pmtBuffer[index];
	info_length = info_length << 8;
	info_length = info_length + pmtBuffer[index + 1];
	info_length = info_length & 0x0FFF;
	
	index = index + 2 + info_length; 
	sec_len -= 13;
	pmtTable->serviceInfoCount = 0;
	
	while (sec_len)
	{
	    currentBufferPosition = (uint8_t *)(pmtBuffer + index);
	    if (pmtTable->serviceInfoCount > MAX_NUMBER_OF_STREAMS_IN_PMT - 1)
        {
            printf("ERROR: %s there is not enough space for Stream info\n", __func__);
            return ERROR;
        }
        
        if (parsePmtServiceInfo(currentBufferPosition, 
            &(pmtTable->pmtServiceInfoArray[pmtTable->serviceInfoCount])) == NO_ERROR)
        {
            // Get info length.
            info_length = pmtBuffer[index + 3];
			info_length = info_length << 8;
			info_length += pmtBuffer[index + 4];
			info_length = info_length & 0x0FFF;
			
			// Check if there is a teletext.
			if (!pmtTable->teletextExist)
			{
			    pmtTable->teletextExist = findTeletext
			        ((currentBufferPosition + 5), info_length);
			}
			
			// Move pointer to new location based on info length.
            index = index + 5 + info_length;
			sec_len -= (5 + info_length);
            pmtTable->serviceInfoCount ++;
        }    
	}	
    
    return NO_ERROR;
}

t_Error printPmtTable(pmt_table_t* pmtTable)
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
        printf("stream_type      |  %d\n",pmtTable->pmtServiceInfoArray[i].streamType);
        printf("pid              |  %d\n",pmtTable->pmtServiceInfoArray[i].PID); 
    }
    printf("\n============= PMT TABLE SECTION =================\n");
    
    return NO_ERROR;
}
