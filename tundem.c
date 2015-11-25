#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "tundem.h"

#define WAIT 10 // number of seconds waiting for frequency locking
#define MHz 1000000 

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t playerHandle;
static uint32_t sourceHandle;
static uint32_t vStreamHandle;
static uint32_t aStreamHandle;

int32_t freqLockCallback(t_LockStatus status)
{
	if (STATUS_ERROR == status)
	{
		printf("ERROR: Error with Tuner_Register_Status_Callback()!\n");
	}
	else
	{
	    // Signalize that we locked to frequency.
	    pthread_mutex_lock(&statusMutex);
        pthread_cond_signal(&statusCondition);
        pthread_mutex_unlock(&statusMutex);
		printf("INFO: Succesfully locked to frequency!\n");
	}
	
	return 0;
}

t_Error initTunerPlayer(uint32_t freq, uint32_t band, t_Module module)
{   
    struct timespec lockStatusWaitTime;
	struct timeval currentTime;
	
	uint32_t frequency = freq * MHz;
	
	gettimeofday(&currentTime,NULL);
    lockStatusWaitTime.tv_sec = currentTime.tv_sec + WAIT;
        
    // Initialize Tuner and check for error. 
    if (ERROR == Tuner_Init())
	{
		printf("ERROR: Error with Tuner_Init()!\n");
		return ERROR;
	}
	
	// Register callback function and check for error. 
	if (ERROR == Tuner_Register_Status_Callback(freqLockCallback))
	{
		printf("ERROR: Error with Tuner_Register_Status_Callback()!\n");
		Tuner_Deinit();
		return ERROR;
	}
	
	// Lock to frequency and check for error.
	if (ERROR == Tuner_Lock_To_Frequency(frequency, band, module))
	{
		printf("ERROR: Eror with Tuner_Lock_To_Frequency()!\n");
		Tuner_Unregister_Status_Callback(freqLockCallback);
		Tuner_Deinit(); 
		return ERROR; 		
	}
	
	// Wait for tuner to lock.
    pthread_mutex_lock(&statusMutex);
    if (ETIMEDOUT == pthread_cond_timedwait
        (&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("ERROR: Error lock timeout exceeded!\n");
        Tuner_Deinit();
        return ERROR;
    }
    pthread_mutex_unlock(&statusMutex);
    
    // Initialize player.
    if (NO_ERROR == Player_Init(&playerHandle))
	{
        printf("INFO: Info Player_Init() success!\n");
    }
    else
    {
        printf("ERROR: Player_Init() fail!\n");
        Tuner_Deinit();
        return ERROR;
    }

    // Open source.
	if (NO_ERROR == Player_Source_Open(playerHandle, &sourceHandle))
	{
        printf("INFO: Player_Source_Open() success!\n");
    }
    else
    {
        printf("ERROR: Player_Source_Open() fail!\n");
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return ERROR;
    }
	
	return NO_ERROR;
}

t_Error deinitTunerPlayer()
{
    // Deinit player
    if (ERROR == Player_Deinit(playerHandle))
    {
        printf("ERROR: Error with Player_Deinit()!\n");
        return ERROR;
    }
    
    // Close source
    if (ERROR == Player_Source_Close(playerHandle, sourceHandle))
    {
        printf("ERROR: Error with Player_Source_Close()!\n");
        return ERROR;
    }
    
    // Unregister callback function.
    if (ERROR == Tuner_Unregister_Status_Callback(freqLockCallback))
    {
        printf("ERROR: Error with Tuner_Unregister_Status_Callback()!\n");
        return ERROR;
    }
    
    // Deinitialize tuner.
	if (ERROR == Tuner_Deinit())
	{
	    printf("ERROR: Error with Tuner_Deinit()!\n");
	    return ERROR;
	}
	
	return NO_ERROR;
}	

t_Error playStream(uint32_t PID, stream_t type)
{
    // Proverimo da li je u pitanju video stream.
    if (type) 
    {
        if (ERROR == Player_Stream_Create(playerHandle, sourceHandle, PID, 
		    VIDEO_TYPE_MPEG2, &vStreamHandle))
	    {
            printf("ERROR: Player_Stream_Create() VIDEO fail!\n");
        }
        else
        {
            printf("INFO: Player_Stream_Create() VIDEO succes!\n");
        }
    }
    // Ili ipak audio stream.
    else
    {
        if (ERROR == Player_Stream_Create(playerHandle, sourceHandle, PID, 
		    AUDIO_TYPE_MPEG_AUDIO, &aStreamHandle))
	    {
            printf("ERROR: Player_Stream_Create() AUDIO fail!\n");
        }
        else
        {
            printf("INFO: Player_Stream_Create() AUDIO succes!\n");
        }
    }            
}

t_Error closeStream(stream_t type)
{
    // Proverimo da li je u pitanju video stream.
    if (type)
    {
        if (ERROR == Player_Stream_Remove(playerHandle, sourceHandle,
            vStreamHandle))
        {
            printf("ERROR: Player_Stream_Remove() VIDEO fail!\n");
        }
        else
        {
            printf("INFO: Player_Stream_Remove() VIDEO succes!\n");
        }
    }
    // Ili je ipak audio stream.
    else
    {
        if (ERROR == Player_Stream_Remove(playerHandle, sourceHandle,
            aStreamHandle))
        {
            printf("ERROR: Player_Stream_Remove() AUDIO fail!\n");
        }
        else
        {
            printf("INFO: Player_Stream_Remove() AUDIO succes!\n");
        }
    }    
}
