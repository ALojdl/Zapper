#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdio.h>
#include <stdint.h>
#include "globals.h"
#include "tdp_api.h"

t_Error initGraphic();
void deinitGraphic();
t_Error drawVolume(uint8_t volume);
t_Error drawInfoBar(uint8_t program, uint8_t teletext, const char *date);
void removeVolume();
void removeInfoBar();

#endif // _GRAPHICS_H
