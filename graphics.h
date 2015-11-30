#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdio.h>
#include <stdint.h>

void initGraphic();
void deinitGraphic();
int32_t drawVolume(uint8_t volume);
void drawInfoBar(uint8_t program, uint8_t teletext);
void removeVolume();
void removeInfoBar();

#endif // _GRAPHICS_H
