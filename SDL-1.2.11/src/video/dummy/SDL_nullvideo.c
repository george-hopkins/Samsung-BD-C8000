/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullmouse_c.h"

#include "GPlayerPorting.h"
#ifdef YAHOO_HW_ACCELERATION
#include "YahooSDLHWAcceleration.h"
#endif

//#define DUMMYVID_DRIVER_NAME "dummy"
#define DUMMYVID_DRIVER_NAME "GPlayer Video"

#ifdef YAHOO_HW_ACCELERATION
#define SAMSUNG_DTV_A1_HW_ACCELERATION
#endif

#if defined(_STI) || defined(_CHELSEA)   || defined(_TRIDENT)
char* org_video_buffer = NULL;
#endif


/* Initialization/Query functions */
static int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void DUMMY_VideoQuit(_THIS);

/* Hardware surface functions */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
/* Hardware Accelation Functions */
static int DUMMY_HWFillRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
static int DUMMY_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst);
static int DUMMY_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
static int DUMMY_HWSWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
static int DUMMY_SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key);
static int DUMMY_SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 value);
#endif

/* DUMMY driver bootstrap functions */

static int DUMMY_Available(void)
{
	/*const char *envr = SDL_getenv("SDL_VIDEODRIVER");
	if ((envr) && (SDL_strcmp(envr, DUMMYVID_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(0); */

	return 1;
}

static void DUMMY_DeleteDevice(SDL_VideoDevice *device)
{
#if defined(_STI) || defined(_CHELSEA)  || defined(_TRIDENT)
	if ( device->hidden->buffer == GetFrameBufferAddress()) 
	{
		 device->hidden->buffer = org_video_buffer;
	}
#endif

	SDL_free(device->hidden);
	SDL_free(device);

}

static SDL_VideoDevice *DUMMY_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = DUMMY_VideoInit;
	device->ListModes = DUMMY_ListModes;
	device->SetVideoMode = DUMMY_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = DUMMY_SetColors;
	//device->UpdateRects = NULL; // DUMMY_UpdateRects;
	device->UpdateRects = DUMMY_UpdateRects;
	device->VideoQuit = DUMMY_VideoQuit;
	device->AllocHWSurface = DUMMY_AllocHWSurface;

#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
	device->CheckHWBlit = DUMMY_CheckHWBlit;
	device->FillHWRect = DUMMY_HWFillRect;
	device->SetHWColorKey = DUMMY_SetHWColorKey;
	device->SetHWAlpha = DUMMY_SetHWAlpha;
#else
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
#endif

	device->LockHWSurface = DUMMY_LockHWSurface;
	device->UnlockHWSurface = DUMMY_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = DUMMY_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = DUMMY_InitOSKeymap;
	device->PumpEvents = DUMMY_PumpEvents;

	device->free = DUMMY_DeleteDevice;

	return device;
}

VideoBootStrap DUMMY_bootstrap = {
	DUMMYVID_DRIVER_NAME, "GPlayer Video",
	DUMMY_Available, DUMMY_CreateDevice
};


int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	/*
	fprintf(stderr, "WARNING: You are using the SDL dummy video driver!\n");
	*/

	/* Determine the screen depth (use default 8-bit depth) */
	/* we change this during the SDL_SetVideoMode implementation... */
	//vformat->BitsPerPixel = 8;
	//vformat->BytesPerPixel = 1;

#ifdef _16BIT_GPLAYER
	vformat ->Rmask = 0x0000F800;
	vformat ->Gmask = 0x000007E0;
	vformat ->Bmask  = 0x0000001F;

	vformat ->BitsPerPixel = 16;
	vformat ->BytesPerPixel = 2;
#else

//	#if defined(_TRIDENT) /* Little endian */
//		vformat ->Amask = 0x000000EF;
//		vformat ->Rmask = 0x0000FF00;
//		vformat ->Gmask = 0x00FF0000;
//		vformat ->Bmask  = 0xFF000000;
//
//		vformat ->BitsPerPixel = 32;
//		vformat ->BytesPerPixel = 4;
//		vformat ->alpha = 128;	/* use 7 bit */
//	#else
		vformat ->Amask = 0xFF000000;
		vformat ->Rmask = 0x00FF0000;
		vformat ->Gmask = 0x0000FF00;
		vformat ->Bmask  = 0x000000FF;

		vformat ->BitsPerPixel = 32;
		vformat ->BytesPerPixel = 4;
		vformat ->alpha = 255;
//	#endif

#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
  this->info.wm_available = 0;
  this->info.hw_available = 1;
  this->info.blit_hw      = 1;
  this->info.blit_hw_CC   = 1;
  this->info.blit_hw_A    = 1;
  this->info.blit_sw      = 1;
  this->info.blit_sw_CC   = 1;
  this->info.blit_sw_A    = 1;
  this->info.blit_fill    = 1;
//  this->info.video_mem    = caps.video_memory / 1024;
  this->info.current_w    = 960;  
  this->info.current_h    = 540;

#endif

#endif

	/* We're done! */
	return(0);
}

SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
   	 return (SDL_Rect **) -1;
}

SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	Uint32 Rmask = 0, Gmask = 0, Bmask = 0, Amask = 0;

	// 디바이스 디스플레이 셋팅 함수 호출.
	VideoInit( width, height, bpp );

#if  defined(_TRIDENT)
  	y_InitData();
#endif

#if defined(_STI) || defined(_CHELSEA)  || defined(_TRIDENT)
	org_video_buffer = this->hidden->buffer;
	this->hidden->buffer =  GetFrameBufferAddress();

	if ( ! this->hidden->buffer ) {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

#else
	if ( this->hidden->buffer ) {
		SDL_free( this->hidden->buffer );
	}

	this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));

	if ( ! this->hidden->buffer ) {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

#endif

	/* 	printf("Setting mode %dx%d\n", width, height); */

	SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));


	switch( bpp )  {
		case 15:
			/* ARGB1555 */
			Amask = 0x00008000;
			Rmask = 0x00007C00;
			Gmask = 0x000002E0;
			Bmask  = 0x0000001F;
			break;
			
		case 16:
			/* RGB656 */
			Rmask = 0x0000F800;
			Gmask = 0x000007E0;
			Bmask  = 0x0000001F;
			break;

		case 32:

//#if defined(_TRIDENT) /* Little endian */
//			/* BGRA8887 */
//			Amask = 0x000000EF;
//			Rmask = 0x0000FF00;
//			Gmask = 0x00FF0000;
//			Bmask  = 0xFF000000;
//#else
			/* ARGB8888 */
			Amask = 0xFF000000;
			Rmask = 0x00FF0000;
			Gmask = 0x0000FF00;
			Bmask  = 0x000000FF;
//#endif
			break;
	}
		

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, Amask ) ) {

#if defined (_STI) || defined(_CHELSEA)  || defined(_TRIDENT)
	if ( this->hidden->buffer == GetFrameBufferAddress()) 
	{
		 this->hidden->buffer = org_video_buffer;
	}

#endif



		SDL_free(this->hidden->buffer);
		this->hidden->buffer = NULL;
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}


	/* Set up the new mode framebuffer */
#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
	current->flags = flags | SDL_FULLSCREEN;
#else
	current->flags = flags & SDL_FULLSCREEN;
#endif
	this->hidden->w = current->w = width;
	this->hidden->h = current->h = height;
	current->pitch = current->w * (bpp / 8);
	current->pixels = this->hidden->buffer;

	//printf( " video set end \n" );
	
	this ->UpdateRects = DUMMY_UpdateRects;

	//printf( " video DUMMY_UpdateRects end \n" );
	
	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
	return y_AllocHWSurface(surface);
#else
	return(-1);
#endif
}
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
	y_FreeHWSurface(surface);
#endif
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	//Uint32	nTickBefore = 0;
	//Uint32	nTickAfter = 0;

	//nTickBefore = SDL_GetTicks();	

	/* do nothing. */
	// too many print
	//printf( "Update Rects : w = %d, h = %d\n", this -> hidden -> w, this -> hidden -> h );

	UpdateSurface( (char*)this->hidden->buffer, this->hidden->w, this->hidden->h );

	//nTickAfter = SDL_GetTicks();	
	//printf("UpdateSurface Time : %d tick\n", (int)nTickAfter - nTickBefore);
}

int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	/* do nothing of note. */
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void DUMMY_VideoQuit(_THIS)
{
#if defined(_STI) || defined(_CHELSEA)  || defined(_TRIDENT)
	if ( this->hidden->buffer == GetFrameBufferAddress()) 
	{
		 this->hidden->buffer = org_video_buffer;
	}

	if (this->screen->pixels == GetFrameBufferAddress()) 
	{
		this->screen->pixels = org_video_buffer;
	}
#endif

	

	if (this->screen->pixels != NULL)
	{
		SDL_free(this->screen->pixels);
		this->screen->pixels = NULL;
	}
#if  defined(_TRIDENT)
  y_freeSurface();
#endif

}

#ifdef SAMSUNG_DTV_A1_HW_ACCELERATION
/* This is HW Accelation Related Function of Samsung DTV */
int DUMMY_HWFillRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
	return y_HWFillRect(dst, rect, color);
}

int DUMMY_CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{

//	int accelerated;

	if((src->flags & SDL_HWSURFACE) != SDL_HWSURFACE &&
		(dst->flags & SDL_HWSURFACE) != SDL_HWSURFACE)
	{
		return (-1);
	}

	src->flags |= SDL_HWACCEL;

	if((src->flags & SDL_SRCCOLORKEY ) == SDL_SRCCOLORKEY)
	{
		src->flags &= ~SDL_HWACCEL;
	}
	if((src->flags & SDL_SRCALPHA) == SDL_SRCALPHA)
	{
		// TODO: Set HW Alpha
	}

	if( (src->flags & SDL_HWSURFACE) == SDL_HWSURFACE &&
		(dst->flags & SDL_HWSURFACE) == SDL_HWSURFACE)
	{
		src->map->hw_blit = DUMMY_HWAccelBlit;
		return 1;
	}
	else if((src->flags & SDL_SWSURFACE) == SDL_SWSURFACE &&
		(dst->flags & SDL_HWSURFACE) == SDL_HWSURFACE)
	{
		src->map->hw_blit = DUMMY_HWSWAccelBlit;
		return 1;
	}
	
	return (-1);
}

int DUMMY_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
	return y_HWAccelBlit(src, srcrect, dst, dstrect);
}

int DUMMY_HWSWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
	return y_HWSWAccelBlit(src, srcrect, dst, dstrect);
}

int DUMMY_SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key)
{
	return y_SetHWColorKey(surface, key);
}

int DUMMY_SetHWAlpha(_THIS, SDL_Surface *surface, Uint8 value)
{
	return y_SetHWAlpha(surface, value);
}

#endif
