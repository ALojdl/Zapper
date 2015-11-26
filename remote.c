#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include "remote.h"


#define NUM 1
#define MAX_CHARACTERS 20

static pthread_t remote;
static int32_t inputFileDesc;
static key_callback_t keyCallFunc;

int32_t getKeys(int32_t count, uint8_t *buf, int32_t *eventsRead)
{    
    // Read next input event and put it in buffer. 
    int32_t ret;
    ret = read(inputFileDesc, buf,
        (size_t)(count * (int)sizeof(struct input_event)));
    if (0 >= ret)
    {
        printf("Error code %d", ret);
        return ERROR;
    }
    
    // Calculate number of read events. 
    *eventsRead = ret / (int)sizeof(struct input_event);
    
    return NO_ERROR;
}

void* remoteFunction()
{
    char deviceName[MAX_CHARACTERS];
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
        
        // Read next input event. 
        if(getKeys(NUM, (uint8_t*)&eventBuf, &eventCnt))
        {
			printf("Error while reading input events !");
			return (void*)ERROR;
		}
		
		// Filter input events. 
        if(eventBuf.type == EV_KEY && (eventBuf.value == EV_VALUE_KEYPRESS || 
            eventBuf.value == EV_VALUE_AUTOREPEAT))
        {
			printf("\nEvent time: %d sec, %d usec\n",
			    (int)eventBuf.time.tv_sec, (int)eventBuf.time.tv_usec);
			printf("Event type: %hu\n", eventBuf.type);
			printf("Event code: %hu\n", eventBuf.code);
			printf("Event value: %d\n", eventBuf.value);
			
			// Pozivam main i gasim thread ako je exit.
			keyCallFunc(eventBuf.code);
			if (eventBuf.code == KEYCODE_EXIT)
			{
			    exit = 1;
			}    
		}
    }
	return (void*)NO_ERROR;
}

t_Error initRemote(key_callback_t keyFunc)
{
    if (pthread_create(&remote, NULL, remoteFunction, NULL))
    {
        printf("ERROR: Error creating remote thread!\n");
		return ERROR;
    }
    keyCallFunc = keyFunc;
    return NO_ERROR;
}

t_Error deinitRemote()
{
	if(pthread_join(remote, NULL))
    {
        printf("ERROR: Error during pthread_join!\n");
		return ERROR;
    }
            
    return NO_ERROR;
} 

