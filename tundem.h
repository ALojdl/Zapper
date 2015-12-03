#ifndef _TUNDEM_H
#define _TUNDEM_H

#include <errno.h>
#include <stdint.h>
#include "tdp_api.h"

typedef enum _tundem_error_t
{
    TUNDEM_ERROR = -1,
    TUNDEM_NO_ERROR
}tundem_error_t;

typedef enum _stream_t
{
    audio, 
    video  
} stream_t;

typedef int32_t(*filter_callback_t)(uint8_t *buffer);

tundem_error_t initTunerPlayer(uint32_t freq, uint32_t band, t_Module module);
tundem_error_t deinitTunerPlayer();
tundem_error_t playStream(uint32_t PID, stream_t type, uint8_t subType);
tundem_error_t closeStream();
tundem_error_t initFilter(uint32_t PID, uint32_t tableID, 
    filter_callback_t callbackFunc);
tundem_error_t deinitFilter(filter_callback_t callbackFunc);
tundem_error_t volumeGet(uint32_t *volume);
tundem_error_t volumeSet(uint32_t volume);

#endif // _TUNDEM_H
