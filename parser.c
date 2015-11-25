#include <stdio.h>
#include <string.h>

#include "parser.h"

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

    patHeader->sectionSyntaxIndicator = (uint8_t) ((*(patHeaderBuffer+1) >> 7) & 0x01);
    patHeader->sectionLength = (uint16_t) (((*(patHeaderBuffer+1) << 8) + 
        *(patHeaderBuffer + 2)) & 0x0FFF);
    patHeader->transportStreamId = (uint16_t) (((*(patHeaderBuffer+3) << 8) + 
        *(patHeaderBuffer + 4)) & 0xFFFF);
    patHeader->versionNumber = (uint8_t) ((*(patHeaderBuffer + 5) >> 1) & 0x1F);
    patHeader->currentNextIndicator = (uint8_t) (*(patHeaderBuffer+5) & 0x01);
    patHeader->sectionNumber = (uint8_t) ((*(patHeaderBuffer + 6) & 0xFF));
    patHeader->lastSectionNumber = (uint8_t) ((*(patHeaderBuffer + 7) & 0xFF));

    return NO_ERROR;
}

t_Error parsePatServiceInfo(const uint8_t* patServiceInfoBuffer, pat_service_info_t* patServiceInfo)
{
    if (patServiceInfoBuffer == NULL || patServiceInfo == NULL)
    {
        printf("ERROR: %s received parameters are not ok.\n", __func__);
        return ERROR;
    }
    
    patServiceInfo->programNumber= (uint16_t) (((*(patServiceInfoBuffer) << 8) + 
        *(patServiceInfoBuffer + 1)) & 0xFFFF);
    patServiceInfo->pid = (uint16_t) (((*(patServiceInfoBuffer+2) << 8) + 
        *(patServiceInfoBuffer + 3)) & 0x1FFF);
    
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
        if (patTable->serviceInfoCount > TABLES_MAX_NUMBER_OF_PIDS_IN_PAT - 1)
        {
            printf("ERROR: %s there is not enough space for Service info\n", __func__);
            return ERROR;
        }
        
        if (parsePatServiceInfo(currentBufferPosition, 
            &(patTable->patServiceInfoArray[patTable->serviceInfoCount])) == NO_ERROR)
        {
            currentBufferPosition += 4; /* Size from program_number to pid */
            parsedLength += 4; /* Size from program_number to pid */
            patTable->serviceInfoCount ++;
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
    
    printf("\n------------------ PAT TABLE SECTION ------------------------\n");
    printf("table_id                 |      %d\n",patTable->patHeader.tableId);
    printf("section_length           |      %d\n",patTable->patHeader.sectionLength);
    printf("transport_stream_id      |      %d\n",patTable->patHeader.transportStreamId);
    printf("section_number           |      %d\n",patTable->patHeader.sectionNumber);
    printf("last_section_number      |      %d\n",patTable->patHeader.lastSectionNumber);
    
    for (i=0; i<patTable->serviceInfoCount; i++)
    {
        printf("-----------------------------------------\n");
        printf("program_number           |      %d\n",patTable->patServiceInfoArray[i].programNumber);
        printf("pid                      |      %d\n",patTable->patServiceInfoArray[i].pid); 
    }
    printf("\n=================== PAT TABLE SECTION =======================\n");
    
    return NO_ERROR;
}
