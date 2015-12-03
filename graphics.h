#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdio.h>
#include <stdint.h>
#include "tdp_api.h"

typedef enum _graphic_error_t
{
    GRAPHIC_ERROR = -1,
    GRAPHIC_NO_ERROR
}graphic_error_t;

typedef struct _info_data_t
{
    uint8_t channelNumber;
    uint8_t teletextExist;  
    uint8_t audioPID;
    uint8_t videoPID;  
    char date[11];
} info_data_t;

t_Error initGraphic();
void deinitGraphic();
t_Error drawVolume(uint8_t volume);
t_Error drawInfoBar(info_data_t data);
void removeVolume();
void removeInfoBar();

#endif // _GRAPHICS_H
