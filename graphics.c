#include <directfb.h>
#include "graphics.h"

#define VOLUME_X_COOR   50
#define VOLUME_Y_COOR   50
#define INFO_BAR_RED    0
#define INFO_BAR_GREEN  166
#define INFO_BAR_BLUE   81
#define TEXT_RED        255
#define TEXT_GREEN      255
#define TEXT_BLUE       255


/* helper macro for error checking */
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

void initGraphic()
{
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
                                
    return;
}

void drawVolume()
{
    /* draw image from file */    
	IDirectFBImageProvider *provider;
	IDirectFBSurface *logoSurface = NULL;
	int32_t logoHeight, logoWidth;
	
    /* create the image provider for the specified file */
	DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, "volume_0.png", &provider));
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
}

void drawInfoBar()
{
    IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
	
    /* rectangle drawing */    
    DFBCHECK(primary->SetColor(primary, INFO_BAR_RED,
        INFO_BAR_GREEN, INFO_BAR_BLUE, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/6, 4*screenHeight/5,
        4*screenWidth/6, screenHeight/6));
        
    // ----------------------- TEXT --------------------------------------------
    DFBCHECK(primary->SetColor(primary, TEXT_RED,
        TEXT_GREEN, TEXT_BLUE, 0xff));
	
	/* specify the height of the font by raising the appropriate flag and setting the height value */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 50;
	
    /* create the font and set the created font for primary surface text drawing */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
    
    /* draw the text */
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ "Text Example",
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ screenWidth/2,
                                 /*y coordinate of the lower left corner of the resulting text*/ 9*screenHeight/10,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
    
    /* switch between the displayed and the work buffer (update the display) */
	DFBCHECK(primary->Flip(primary, NULL, 0));
}

void removeInfoBar()
{
    IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
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
	
    return;
}

void removeVolume()
{
    IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
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
	
    return;
}

void deinitGraphic()
{
    primary->Release(primary);
	dfbInterface->Release(dfbInterface);
    return;
}

