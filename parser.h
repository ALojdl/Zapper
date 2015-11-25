#ifndef _PARSER_H
#define _PARSER_H

#include <stdint.h>

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

void getConfiguration(const char *path, init_data_t *data);

#endif // _PARSER_H
