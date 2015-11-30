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
static int screenWidth = 0;
static int screenHeight = 0;
static DFBSurfaceDescription surfaceDesc;

static timer_t volumeTimerId;
static timer_t infoTimerId;

static struct itimerspec volumeTimerSpec;
static struct itimerspec volumeTimerSpecOld;
 
static struct itimerspec infoTimerSpec;
static struct itimerspec infoTimerSpecOld;


void initGraphic()
{
    struct sigevent volumeSignalEvent;
    struct sigevent infoSignalEvent;
    int32_t ret;
    
    
    /* initialize DirectFB */    
	DFBCHECK(DirectFBInit(NULL, NULL));	
    /* fetch the DirectFB interface */
	DFBCHECK(DirectFBCreate(&dfbInterface));
    /* tell the DirectFB to take the full screen for this application */
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
    
    /* create primary surface with double buffering enabled */    
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    /* fetch the screen size */
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));
    
    
    /* clear the screen before drawing anything (draw black full screen rectangle)*/    
    DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
                               /*red*/ 0x00,
                               /*green*/ 0x00,
                               /*blue*/ 0x00,
                               /*alpha*/ 0x00));
                               
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
                                    /*upper left x coordinate*/ 0,
                                    /*upper left y coordinate*/ 0,
                                    /*rectangle width*/ screenWidth,
                                    /*rectangle height*/ screenHeight));
                                    
    // -------------------------------------------------------------------------
    // PREPARE TIMERS
    // -------------------------------------------------------------------------
    
    // ----------------------------- VOLUME ------------------------------------
    // Set timer.
	memset(&volumeTimerSpec, 0, sizeof(volumeTimerSpec));
	
	volumeTimerSpec.it_value.tv_sec = VOLUME_TIME;
	volumeTimerSpec.it_value.tv_nsec = 0;
	
	 /* create timer */
    volumeSignalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    volumeSignalEvent.sigev_notify_function = removeVolume; /* function to be called when timer runs out */
    volumeSignalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
    volumeSignalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&volumeSignalEvent,
                       /*where to store the ID of the newly created timer*/&volumeTimerId);
    if(ret == -1){
        printf("ERROR: Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return; // Add error notification.
    }
    
    // -----------------------------  INFO  ------------------------------------
    
    // Set timer.
	memset(&infoTimerSpec, 0, sizeof(infoTimerSpec));
	
	infoTimerSpec.it_value.tv_sec = VOLUME_TIME;
	infoTimerSpec.it_value.tv_nsec = 0;
	
	 /* create timer */
    infoSignalEvent.sigev_notify = SIGEV_THREAD; /* tell the OS to notify you about timer by calling the specified function */
    infoSignalEvent.sigev_notify_function = removeInfoBar; /* function to be called when timer runs out */
    infoSignalEvent.sigev_value.sival_ptr = NULL; /* thread arguments */
    infoSignalEvent.sigev_notify_attributes = NULL; /* thread attributes (e.g. thread stack size) - if NULL default attributes are applied */
    ret = timer_create(/*clock for time measuring*/CLOCK_REALTIME,
                       /*timer settings*/&infoSignalEvent,
                       /*where to store the ID of the newly created timer*/&infoTimerId);
    if(ret == -1){
        printf("ERROR: Error creating timer, abort!\n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        
        return; // Add error notification.
    }
                                
    return;
}

int32_t drawVolume(uint8_t volume)
{
    /* draw image from file */    
	IDirectFBImageProvider *provider;
	IDirectFBSurface *logoSurface = NULL;
	int32_t logoHeight, logoWidth, ret;	
	char imageName[15];

	int32_t timerFlags = 0;
	
	// Prepare timer 
	
	// Write name of image depending on current volume.
	sprintf(imageName, "volume_%hu.png", volume);
	
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
    /* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, imageName, &provider));
    /* get surface descriptor for the surface where the image will be rendered */
	DFBCHECK(provider->GetSurfaceDescription(provider, &surfaceDesc));
    /* create the surface for the image */
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &logoSurface));
    /* render the image to the surface */
	DFBCHECK(provider->RenderTo(provider, logoSurface, NULL));
	
    /* cleanup the provider after rendering the image to the surface */
	provider->Release(provider);
	
    /* fetch the logo size and add (blit) it to the screen */
	DFBCHECK(logoSurface->GetSize(logoSurface, &logoWidth, &logoHeight));
	DFBCHECK(primary->Blit
	    (primary, logoSurface, NULL, VOLUME_X_COOR, VOLUME_Y_COOR));
    
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// -------------------------------------------------------------------------
	// SPECIFY TIMER
    
    /* specify the timer timeout time */
    volumeTimerSpec.it_value.tv_sec = VOLUME_TIME;
    volumeTimerSpec.it_value.tv_nsec = 0;
    
    /* set the new timer specs */
    ret = timer_settime(volumeTimerId,0,&volumeTimerSpec,&volumeTimerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
	
	return 0;
}

void drawInfoBar(uint8_t program, uint8_t teletext)
{
    IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	int32_t ret;
	char prog[11];      // "Program XX"
	char datum[32];     // "Naziv dana u nedelji mm/dd/yyyy"
	char telxt[20];     // "Telekst ne postoji"
	
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
    /* rectangle drawing */    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));
    
    // -------------------------------------------------------------------------    
    // ----------------------- TEXT --------------------------------------------
    DFBCHECK(primary->SetColor(primary, TEXT_RED,
        TEXT_GREEN, TEXT_BLUE, 0xff));
	
	/* specify the height of the font by raising the appropriate flag and setting the height value */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 40;
	
    /* create the font and set the created font for primary surface text drawing */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
    
    // WRITE PROGRAM NUMBER
    sprintf(prog, "Program %hu", program);
    
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ prog,
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ screenWidth/5,
                                 /*y coordinate of the lower left corner of the resulting text*/ 17*screenHeight/20,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
                                 
    // WRITE DATE
    sprintf(datum, "Naziv dana u nedelji mm/dd/yyyy");
    
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ datum,
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ screenWidth/5,
                                 /*y coordinate of the lower left corner of the resulting text*/ 19*screenHeight/20,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
                                 
    // WRITE TELETEXT
    if (teletext)
    {
        sprintf(telxt, "Teletekst postoji");    
    }
    else
    {
        sprintf(telxt, "Teletekst ne postoji");    
    }
    
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ telxt,
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ 5*screenWidth/9,
                                 /*y coordinate of the lower left corner of the resulting text*/ 17*screenHeight/20,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// -------------------------------------------------------------------------
	// SPECIFY TIMER
    
    /* specify the timer timeout time */
    infoTimerSpec.it_value.tv_sec = INFO_TIME;
    infoTimerSpec.it_value.tv_nsec = 0;
    
    /* set the new timer specs */
    ret = timer_settime(infoTimerId,0,&infoTimerSpec,&infoTimerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
	
	return;
}

void removeInfoBar()
{
    IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	int32_t ret;
	
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// TO DO -------------------------------------------------------------------
	/* rectangle drawing */    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));
	// -------------------------------------------------------------------------
	
	
	/* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	/* stop the timer */
    memset(&infoTimerSpec,0,sizeof(infoTimerSpec));
    ret = timer_settime(infoTimerId,0,&infoTimerSpec,&infoTimerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
	
	
    return;
}

void removeVolume(union sigval signalArg)
{	
    int32_t ret;
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	// TO DO -------------------------------------------------------------------
	/* rectangle drawing */    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0x00));
    DFBCHECK(primary->FillRectangle(primary, VOLUME_X_COOR, VOLUME_Y_COOR,
        200, 200));
	// -------------------------------------------------------------------------
	
	/* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
	/* stop the timer */
    memset(&volumeTimerSpec,0,sizeof(volumeTimerSpec));
    ret = timer_settime(volumeTimerId,0,&volumeTimerSpec,&volumeTimerSpecOld);
    if(ret == -1){
        printf("Error setting timer in %s!\n", __FUNCTION__);
    }
	
    return;
}

void deinitGraphic()
{
    timer_delete(volumeTimerId);
    timer_delete(infoTimerId);
    primary->Release(primary);
	dfbInterface->Release(dfbInterface);
    return;
}

