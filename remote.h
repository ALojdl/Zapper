#ifndef _REMOTE_H
#define _REMOTE_H

#include <stdint.h>
#include "tdp_api.h"

#define KEYCODE_P_MINUS 	61
#define KEYCODE_P_PLUS 		62
#define KEYCODE_V_PLUS      63
#define KEYCODE_V_MINUS     64
#define KEYCODE_MUTE        60
#define KEYCODE_EXIT 		102
#define KEYCODE_INFO 		358

#define EV_VALUE_RELEASE    0
#define EV_VALUE_KEYPRESS   1
#define EV_VALUE_AUTOREPEAT 2

typedef enum _remote_error_t
{
    REMOTE_ERROR = -1,
    REMOTE_NO_ERROR
}remote_error_t;

typedef int32_t (*key_callback_t)(uint16_t keyPressed);

remote_error_t initRemote();
void registerRemoteCallback(key_callback_t keyFunc);
remote_error_t deinitRemote();

#endif // _REMOTE_H
