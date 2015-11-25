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

typedef int32_t (*keyCallback_t)(uint16_t keyPressed);

t_Error initRemote(keyCallback_t keyFunc);
t_Error deinitRemote();

#endif // _REMOTE_H
