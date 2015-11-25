#ifndef _PARSER_H
#define _PARSER_H

#include <stdint.h>
#include "tdp_api.h"

#define TABLES_MAX_NUMBER_OF_PIDS_IN_PAT 20

typedef struct _init_data_t
{
    uint32_t freq;
    uint32_t band;
    char module[8];
    uint16_t aPID;
    uint16_t vPID;
    char aType[8];
    char vType[8];
} init_data_t;

typedef struct _pat_header_t
{
    uint8_t tableId;
    uint8_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t transportStreamId;
    uint8_t versionNumber;
    uint8_t currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber; 
} pat_header_t;

typedef struct _pat_service_info_t
{    
    uint16_t    programNumber;
    uint16_t    pid;
} pat_service_info_t;

typedef struct _pat_table_t
{    
    pat_header_t patHeader;
    pat_service_info_t patServiceInfoArray[TABLES_MAX_NUMBER_OF_PIDS_IN_PAT];
    uint8_t serviceInfoCount;
} pat_table_t;

void getConfiguration(const char *path, init_data_t *data);
t_Error parsePatTable(const uint8_t* patSectionBuffer, pat_table_t* patTable);
t_Error printPatTable(pat_table_t* patTable);

#endif // _PARSER_H
