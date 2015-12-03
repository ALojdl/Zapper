#include <directfb.h>
#include <signal.h>
#include <time.h>
#include "graphics.h"

#define VOLUME_X_COOR   50
#define VOLUME_Y_COOR   50
#define INFO_BAR_RED    0
#define INFO_BAR_GREEN  166
#define INFO_BAR_BLUE   81
#define TEXT_RED        255
#define TEXT_GREEN      255
#define TEXT_BLUE       255
#define VOLUME_TIME     3
#define INFO_TIME       5


// Helper macro for error checking. 
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}

static IDirectFBSurface *primary = NULL;
static IDirectFB *dfbInterface = NULL;
static int screenWidth;
static int screenHeight;
static DFBSurfaceDescription surfaceDesc;

static timer_t volumeTimerId;
static timer_t infoTimerId;

static struct itimerspec volumeTimerSpec;
static struct itimerspec volumeTimerSpecOld;
static struct itimerspec infoTimerSpec;
static struct itimerspec infoTimerSpecOld;


t_Error initGraphic()
{
    struct sigevent volumeSignalEvent;
    struct sigevent infoSignalEvent;    
    
    // Initialize DirectFB.     
	DFBCHECK(DirectFBInit(NULL, NULL));	
    DFBCHECK(DirectFBCreate(&dfbInterface));
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
    
    // Create primary surface, with enabled double buffering.     
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface,
	    &surfaceDesc, &primary));    
    
    // Fetch the screen size and clear screen.
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));   
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));                               
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));
                                    
    // Prepare volume timer specification.
	memset(&volumeTimerSpec, 0, sizeof(volumeTimerSpec));	
	volumeTimerSpec.it_value.tv_sec = VOLUME_TIME;
	volumeTimerSpec.it_value.tv_nsec = 0;
	
	// Prepare volume signal event specification.
    volumeSignalEvent.sigev_notify = SIGEV_THREAD; 
    volumeSignalEvent.sigev_notify_function = removeVolume; 
    volumeSignalEvent.sigev_value.sival_ptr = NULL; 
    volumeSignalEvent.sigev_notify_attributes = NULL; 
    
    // Create volume timer and check if there was error while creating. 
    if (-1 == timer_create(CLOCK_REALTIME, &volumeSignalEvent, &volumeTimerId))
    {
        printf("ERROR: %s failed to create volume timer.\n", __func__);
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);        
        return ERROR;
    }
    
    // Prepare info timer specification. 
	memset(&infoTimerSpec, 0, sizeof(infoTimerSpec));	
	infoTimerSpec.it_value.tv_sec = VOLUME_TIME;
	infoTimerSpec.it_value.tv_nsec = 0;
	
	// Prepare info signal event specification.
    infoSignalEvent.sigev_notify = SIGEV_THREAD;
    infoSignalEvent.sigev_notify_function = removeInfoBar;
    infoSignalEvent.sigev_value.sival_ptr = NULL; 
    infoSignalEvent.sigev_notify_attributes = NULL;
    
    // Create info timer and check if there was error while creating. 
    if(-1 == timer_create(CLOCK_REALTIME, &infoSignalEvent, &infoTimerId))
    {
        printf("ERROR: %s failed to create info timer.\n", __func__);
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        return ERROR;
    }
                                
    return NO_ERROR;
}

t_Error drawVolume(uint8_t volume)
{  
	IDirectFBImageProvider *provider;
	IDirectFBSurface *logoSurface;
	char imageName[15];
	int32_t logoHeight;
	int32_t logoWidth;
	int32_t timerFlags = 0;	
	
	// Make a new name.
	sprintf(imageName, "volume_%hu.png", volume);
	
    // Read image, prepare surface descriptor and render image to given surface.
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, imageName, &provider));
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	provider->Release(provider);
	
    // Fetch the logo size and add it to the screen buffer. 
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary, logoSurface,
	    NULL, VOLUME_X_COOR, VOLUME_Y_COOR));    
    
    // Switch buffers.
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// Fetch the logo size and add it to the second screen buffer. 
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit(primary, logoSurface,
	    NULL, VOLUME_X_COOR, VOLUME_Y_COOR));  
    
    // Specify the timer timeout time and set it, saving the old specifications. 
    volumeTimerSpec.it_value.tv_sec = VOLUME_TIME;
    volumeTimerSpec.it_value.tv_nsec = 0;
    
    if (-1 == timer_settime(volumeTimerId, 0, 
        &volumeTimerSpec, &volumeTimerSpecOld))
    {
        printf("ERROR: %s failed while setting timer.\n", __func__);
        return ERROR;
    }
	
	return NO_ERROR;
}

t_Error drawInfoBar(info_data_t data)
{
    IDirectFBFont *fontInterface;
	DFBFontDescription fontDesc;
	char prog[32];
	char date[32];
	char telxt[32];
	char audio[32];
	char video[32];
	
	// Prepare strings for info bar.
	sprintf(prog, "Program %hu", data.channelNumber);
	sprintf(date, "Naziv dana u nedelji %s", data.date);
	sprintf(audio, "Audio PID %hu", data.audioPID);
	sprintf(video, "Video PID %hu", data.videoPID);
	if (data.teletextExist)
    {
        sprintf(telxt, "Teletekst postoji");    
    }
    else
    {
        sprintf(telxt, "Teletekst ne postoji");    
    }
    
    // Prepare text font, color and size. Then create font and draw it.
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 40;
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, 
	    "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));    
	
	// Draw a rectangle representing info bar.    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED, 
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));

    // Write program number, date and teletext to info bar.
    DFBCHECK(primary->SetColor(primary, TEXT_RED, TEXT_GREEN, TEXT_BLUE, 0xff));
    DFBCHECK(primary->DrawString(primary, prog, -1, screenWidth/5, 
	    17*screenHeight/20, DSTF_LEFT));
	DFBCHECK(primary->DrawString(primary, audio, -1, screenWidth/5, 
	    18*screenHeight/20, DSTF_LEFT));
	DFBCHECK(primary->DrawString(primary, video, -1, screenWidth/5, 
	    19*screenHeight/20, DSTF_LEFT));
	DFBCHECK(primary->DrawString(primary, date, -1, 4*screenWidth/9, 
	    19*screenHeight/20, DSTF_LEFT));  
	DFBCHECK(primary->DrawString(primary, telxt, -1, 4*screenWidth/9, 
	    17*screenHeight/20, DSTF_LEFT));
    
    // Switch buffers.
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// Draw a rectangle representing info bar to second buffer.    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED, 
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));

    // Write program number, date and teletext to info bar on second buffer.
    DFBCHECK(primary->SetColor(primary, TEXT_RED, TEXT_GREEN, TEXT_BLUE, 0xff));
    DFBCHECK(primary->DrawString(primary, prog, -1, screenWidth/5, 
	    17*screenHeight/20, DSTF_LEFT));
	DFBCHECK(primary->DrawString(primary, date, -1, screenWidth/5, 
	    19*screenHeight/20, DSTF_LEFT));  
	DFBCHECK(primary->DrawString(primary, telxt, -1, 5*screenWidth/9, 
	    17*screenHeight/20, DSTF_LEFT));
    
    // Specify the timer timeout time and set new timer.
    infoTimerSpec.it_value.tv_sec = INFO_TIME;
    infoTimerSpec.it_value.tv_nsec = 0;    
    
    if (-1 == timer_settime(infoTimerId, 0, &infoTimerSpec, &infoTimerSpecOld))
    {
        printf("ERROR: %s failed while creating timer.\n", __func__);
        return ERROR;
    }
	
	return NO_ERROR;
}

void removeInfoBar()
{		
	// Draw transparent rectangle in place of info bar.  
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED, 
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));
	
	// Switch buffers.
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// Draw transparent rectangle in place of info bar on second buffer.  
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED, 
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));
	
	// Stop timer.
    memset(&infoTimerSpec, 0, sizeof(infoTimerSpec));
    if (-1 == timer_settime(infoTimerId, 0, &infoTimerSpec, &infoTimerSpecOld))
    {
        printf("ERROR: %s failed while stoping timer.\n", __func__);
    }
}

void removeVolume(union sigval signalArg)
{	    	
	// Draw transparent rectangle in place of volume bar.  
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, VOLUME_X_COOR, VOLUME_Y_COOR,
        200, 200));
	
	// Switch buffers.
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// Draw transparent rectangle in place of volume bar on second buffer.  
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, VOLUME_X_COOR, VOLUME_Y_COOR,
        200, 200));
	
	// Stop timer.
    memset(&volumeTimerSpec, 0, sizeof(volumeTimerSpec));
    
    if (-1 == timer_settime(volumeTimerId, 0, &volumeTimerSpec, 
        &volumeTimerSpecOld))
    {
        printf("ERROR: %s failed while stoping timer.\n", __func__);
    }
}

void deinitGraphic()
{
    timer_delete(volumeTimerId);
    timer_delete(infoTimerId);
    primary->Release(primary);
	dfbInterface->Release(dfbInterface);
    return;
}

