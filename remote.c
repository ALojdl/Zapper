#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include "remote.h"
#include "tdp_api.h"

#define NUM 1

static pthread_t remote;
static int32_t inputFileDesc;

int32_t getKeys(int32_t count, uint8_t* buf, int32_t* eventsRead)
{
    int32_t ret = 0;
    
    /* read next input event and put it in buffer */
    ret = read(inputFileDesc, buf, (size_t)(count * (int)sizeof(struct input_event)));
    if(ret <= 0)
    {
        printf("Error code %d", ret);
        return ERROR;
    }
    /* calculate number of read events */
    *eventsRead = ret / (int)sizeof(struct input_event);
    
    return NO_ERROR;
}

void* remoteFunction()
{
    char deviceName[20];
    struct input_event eventBuf;
    uint32_t eventCnt;
    int32_t counter = 0;
    uint8_t exit = 0;
    const char* dev = "/dev/input/event0";
    
    inputFileDesc = open(dev, O_RDWR);
    if(inputFileDesc == -1)
    {
        printf("ERROR: Error while opening device (%s) !", strerror(errno));
		return (void*)ERROR;
    }
        
    while(!exit){
        
        /* read next input event */
        if(getKeys(NUM, (uint8_t*)&eventBuf, &eventCnt))
        {
			printf("Error while reading input events !");
			return (void*)ERROR;
		}
		
		/* filter input events */
        if(eventBuf.type == EV_KEY && 
          (eventBuf.value == EV_VALUE_KEYPRESS || eventBuf.value == EV_VALUE_AUTOREPEAT))
        {
			printf("Event time: %d sec, %d usec\n",(int)eventBuf.time.tv_sec,(int)eventBuf.time.tv_usec);
			printf("Event type: %hu\n",eventBuf.type);
			printf("Event code: %hu\n",eventBuf.code);
			printf("Event value: %d\n",eventBuf.value);
			printf("\n");
			
			switch(eventBuf.code)
			{
				case KEYCODE_INFO: 
					printf("\nCounter: %d \n\n", counter);
					break;
				case KEYCODE_P_PLUS:
					counter++;
					break;
				case KEYCODE_P_MINUS:
					counter--;
					break;
				case KEYCODE_EXIT:
					exit = 1;
					break;
				default:
					printf("\nPress P+, P-, info or exit! \n\n");
			}
		}
    }
	return (void*)NO_ERROR;
}

t_Error initRemote()
{
    if (pthread_create(&remote, NULL, remoteFunction, NULL))
    {
        printf("ERROR: Error creating remote thread!\n");
		return ERROR;
    }
    return NO_ERROR;
}
// Fali funkcija za gasenje. 

