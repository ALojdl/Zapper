#ifndef _PARSER_H
#define _PARSER_H

#include <stdint.h>
#include "globals.h"
#include "tdp_api.h"

#define MAX_NUMBER_OF_PIDS_IN_PAT       30
#define MAX_NUMBER_OF_STREAMS_IN_PMT    30     
#define AUDIO_TYPE_DOLBY_AC3 	        1
#define AUDIO_TYPE_MPEG_AUDIO           10
#define VIDEO_TYPE_MPEG4                41
#define VIDEO_TYPE_MPEG2                42

// Config file structure. 
typedef struct _init_data_t
{
    uint32_t freq;
    uint32_t band;
    uint16_t audioPID;
    uint16_t videoPID;
    uint8_t module;
    uint8_t audioType;
    uint8_t videoType;
} init_data_t;

// PAT table structures.
typedef struct _pat_header_t
{
    uint8_t tableId;
    uint8_t versionNumber;
    uint16_t sectionLength;
} pat_header_t;

typedef struct _pat_service_info_t
{    
    uint16_t    programNumber;
    uint16_t    PID;
} pat_service_info_t;

typedef struct _pat_table_t
{    
    pat_header_t patHeader;
    pat_service_info_t patServiceInfoArray[MAX_NUMBER_OF_PIDS_IN_PAT];
    uint8_t serviceInfoCount;
} pat_table_t;

// PMT table structures.
typedef struct _pmt_header_t
{
   uint8_t tableId;
   uint8_t versionNumber;
   uint16_t sectionLength;
   uint16_t programNumber;
} pmt_header_t;

typedef struct _pmt_stream_t 
{
    uint16_t PID;
    uint8_t streamType;
} pmt_stream_t;

typedef struct _pmt_table_t 
{
    pmt_header_t pmtHeader;
    pmt_stream_t pmtServiceInfoArray[MAX_NUMBER_OF_STREAMS_IN_PMT];
    uint8_t serviceInfoCount;
    uint8_t teletextExist;
} pmt_table_t;

typedef struct _tot_table_t
{
    uint8_t tableId;
    uint16_t sectionLength;
} tot_table_t;

void getConfiguration(const char *path, init_data_t *data);
t_Error parsePatTable(const uint8_t *patSectionBuffer, pat_table_t *patTable);
t_Error printPatTable(pat_table_t *patTable);
t_Error parsePmtTable(const uint8_t *pmtBuffer, pmt_table_t *pmtTable);
t_Error printPmtTable(pmt_table_t *pmtTable);
t_Error parseTotTable(const uint8_t *totTable, char *date);

#endif // _PARSER_H
