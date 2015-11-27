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

typedef int32_t(*filter_callback_t)(uint8_t *buffer);

t_Error initTunerPlayer(uint32_t freq, uint32_t band, t_Module module);
t_Error deinitTunerPlayer();
t_Error playStream(uint32_t PID, stream_t type);
t_Error closeStream();
t_Error initFilter(uint32_t PID, uint32_t tableID, 
    filter_callback_t callbackFunc);
t_Error deinitFilter(filter_callback_t callbackFunc);
t_Error volumeGet(uint32_t *volume);
t_Error volumeSet(uint32_t volume);

#endif // _TUNDEM_H
