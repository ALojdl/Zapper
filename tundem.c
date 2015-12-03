#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include "tundem.h"

#define DEBUG_INFO

#define WAIT 10     // Number of seconds waiting for frequency locking.
#define MHz 1000000 // Multiplication constant for transform MHz -> Hz

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t playerHandle;
static uint32_t sourceHandle;
static uint32_t filterHandle;

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
#ifdef DEBUG_INFO
		printf("INFO: Succesfully locked to frequency!\n");
#endif
	}
	
	return 0;
}

tundem_error_t initTunerPlayer(uint32_t freq, uint32_t band, t_Module module)
{   
    struct timespec lockStatusWaitTime;
	struct timeval currentTime;
	
	gettimeofday(&currentTime,NULL);
    lockStatusWaitTime.tv_sec = currentTime.tv_sec + WAIT;
        
    // Initialize Tuner and check for error. 
    if (ERROR == Tuner_Init())
	{
		printf("ERROR: Error with Tuner_Init()!\n");
		return TUNDEM_ERROR;
	}
	
	// Register callback function and check for error. 
	if (ERROR == Tuner_Register_Status_Callback(freqLockCallback))
	{
		printf("ERROR: Error with Tuner_Register_Status_Callback()!\n");
		Tuner_Deinit();
		return TUNDEM_ERROR;
	}
	
	// Lock to frequency and check for error.
	if (ERROR == Tuner_Lock_To_Frequency(freq * MHz, band, module))
	{
		printf("ERROR: Eror with Tuner_Lock_To_Frequency()!\n");
		Tuner_Unregister_Status_Callback(freqLockCallback);
		Tuner_Deinit(); 
		return TUNDEM_ERROR; 		
	}
	
	// Wait for tuner to lock.
    pthread_mutex_lock(&statusMutex);
    if (ETIMEDOUT == pthread_cond_timedwait
        (&statusCondition, &statusMutex, &lockStatusWaitTime))
    {
        printf("ERROR: %s lock timeout exceeded!\n", __func__);
        Tuner_Deinit();
        return TUNDEM_ERROR;
    }
    pthread_mutex_unlock(&statusMutex);
    
    // Initialize player.
    if (NO_ERROR == Player_Init(&playerHandle))
	{
#ifdef DEBUG_INFO
        printf("INFO: Player_Init() success!\n");
#endif
    }
    else
    {
        printf("ERROR: Player_Init() fail!\n");
        Tuner_Deinit();
        return TUNDEM_ERROR;
    }

    // Open source.
	if (NO_ERROR == Player_Source_Open(playerHandle, &sourceHandle))
	{
#ifdef DEBUG_INFO
        printf("INFO: Player_Source_Open() success!\n");
#endif 
    }
    else
    {
        printf("ERROR: Player_Source_Open() fail!\n");
		Player_Deinit(playerHandle);
        Tuner_Deinit();
        return TUNDEM_ERROR;
    }
	
	return TUNDEM_NO_ERROR;
}

tundem_error_t deinitTunerPlayer()
{    
    // Close source.
    if (ERROR == Player_Source_Close(playerHandle, sourceHandle))
    {
        printf("ERROR: Error with Player_Source_Close()!\n");
        return TUNDEM_ERROR;
    }
    
    // Deinit player.
    if (ERROR == Player_Deinit(playerHandle))
    {
        printf("ERROR: Error with Player_Deinit()!\n");
        return TUNDEM_ERROR;
    }
    
    // Unregister callback function.
    if (ERROR == Tuner_Unregister_Status_Callback(freqLockCallback))
    {
        printf("ERROR: Error with Tuner_Unregister_Status_Callback()!\n");
        return TUNDEM_ERROR;
    }
    
    // Deinitialize tuner.
	if (ERROR == Tuner_Deinit())
	{
	    printf("ERROR: Error with Tuner_Deinit()!\n");
	    return TUNDEM_ERROR;
	}
	
	return TUNDEM_NO_ERROR;
}	

tundem_error_t playStream(uint32_t PID, stream_t type, uint8_t subType)
{
    // Proverimo da li je u pitanju video stream.
    if (type) 
    {
        if (ERROR == Player_Stream_Create(playerHandle, sourceHandle, PID, 
		    subType, &vStreamHandle))
	    {
            printf("ERROR: Player_Stream_Create() VIDEO fail!\n");
            return TUNDEM_ERROR;
        }
        else
        {
#ifdef DEBUG_INFO
            printf("INFO: Player_Stream_Create() VIDEO succes!\n");
#endif
            return TUNDEM_NO_ERROR;
        }
    }
    // Ili ipak audio stream.
    else
    {
        if (ERROR == Player_Stream_Create(playerHandle, sourceHandle, PID, 
		    subType, &aStreamHandle))
	    {
            printf("ERROR: Player_Stream_Create() AUDIO fail!\n");
            return TUNDEM_ERROR;
        }
        else
        {
#ifdef DEBUG_INFO
            printf("INFO: Player_Stream_Create() AUDIO succes!\n");
#endif
            return TUNDEM_NO_ERROR;
        }
    }            
}

tundem_error_t closeStream(stream_t type)
{
    // Proverimo da li je u pitanju video stream.
    if (type)
    {
        if (ERROR == Player_Stream_Remove(playerHandle, sourceHandle,
            vStreamHandle))
        {
            printf("ERROR: Player_Stream_Remove() VIDEO fail!\n");
            return TUNDEM_ERROR;
        }
        else
        {
#ifdef DEBUG_INFO
            printf("INFO: Player_Stream_Remove() VIDEO succes!\n");
#endif
            return TUNDEM_NO_ERROR;
        }
    }
    // Ili je ipak audio stream.
    else
    {
        if (ERROR == Player_Stream_Remove(playerHandle, sourceHandle,
            aStreamHandle))
        {
            printf("ERROR: Player_Stream_Remove() AUDIO fail!\n");
            return TUNDEM_ERROR;
        }
        else
        {
#ifdef DEBUG_INFO
            printf("INFO: Player_Stream_Remove() AUDIO succes!\n");
#endif 
            return TUNDEM_NO_ERROR;
        }
    }    
}

tundem_error_t initFilter(uint32_t PID, uint32_t tableID, filter_callback_t callbackFunc)
{	
    if (ERROR == Demux_Set_Filter(playerHandle, PID, tableID, &filterHandle))
    {
        printf("ERROR: %s failed to set filter().\n", __func__);
        return TUNDEM_ERROR;
    }
    
    if (ERROR == Demux_Register_Section_Filter_Callback(callbackFunc))
    {
        printf("ERROR: %s failed to register callback function.\n", __func__);
        return TUNDEM_ERROR;
    }
    
    return TUNDEM_NO_ERROR;
}

tundem_error_t deinitFilter(filter_callback_t callbackFunc)
{
    if (ERROR == Demux_Unregister_Section_Filter_Callback(callbackFunc))
    {
        printf("ERROR: %s failed to unregister callback function.\n", __func__);
        return TUNDEM_ERROR;
    }
    
    if (ERROR == Demux_Free_Filter(playerHandle, filterHandle))
    {
        printf("ERROR: %s failed to free filter.\n", __func__);
        return TUNDEM_ERROR;
    }
        
    return TUNDEM_NO_ERROR;
}

tundem_error_t volumeGet(uint32_t *volume)
{   
	if (ERROR == Player_Volume_Get(playerHandle, volume))
	{
	    printf("ERROR: Player_Volume_Get() breaked.\n");
	    return TUNDEM_ERROR;
	}
	
	return TUNDEM_NO_ERROR;
}

tundem_error_t volumeSet(uint32_t volume)
{
	if (ERROR == Player_Volume_Set(playerHandle, volume))
	{
	    printf("ERROR: Player_Volume_Set() breaked.\n");
	    return TUNDEM_ERROR;
	}
	
	return TUNDEM_NO_ERROR;
}
