#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdio.h>
#include <stdint.h>
#include "globals.h"
#include "tdp_api.h"

typedef struct _info_data_t
{
    char date[11];
    uint8_t channelNumber;
    uint8_t teletextExist;    
} info_data_t;

t_Error initGraphic();
void deinitGraphic();
t_Error drawVolume(uint8_t volume);
t_Error drawInfoBar(info_data_t data);
void removeVolume();
void removeInfoBar();

#endif // _GRAPHICS_H
