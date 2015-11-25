#ifndef _TUNDEM_H
#define _TUNDEM_H

#include <errno.h>
#include <stdint.h>
#include "tdp_api.h"

typedef enum _stream_t
{
    audio, 
    video  
} stream_t;

t_Error initTunerPlayer(uint32_t freq, uint32_t band, t_Module module);
t_Error deinitTunerPlayer();
t_Error playStream(uint32_t PID, stream_t type);
t_Error closeStream();

#endif // _TUNDEM_H
