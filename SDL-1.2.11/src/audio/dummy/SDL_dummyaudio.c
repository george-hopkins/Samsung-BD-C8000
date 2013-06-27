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

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to nowhere... */

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_dummyaudio.h"

#include "GPlayerPorting.h"

/* The tag name used by DUMMY audio */
//#define DUMMYAUD_DRIVER_NAME         "dummy"
#define DUMMYAUD_DRIVER_NAME         "GPlayer Audio"

/* Audio driver functions */
static int DUMMYAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DUMMYAUD_WaitAudio(_THIS);
static void DUMMYAUD_PlayAudio(_THIS);
static Uint8 *DUMMYAUD_GetAudioBuf(_THIS);
static void DUMMYAUD_CloseAudio(_THIS);

/* Audio driver bootstrap functions */
static int DUMMYAUD_Available(void)
{
	/*const char *envr = SDL_getenv("SDL_AUDIODRIVER");
	if (envr && (SDL_strcmp(envr, DUMMYAUD_DRIVER_NAME) == 0)) {
		return(1);
	}
	return(0); */

	return(1);
}

static void DUMMYAUD_DeleteDevice(SDL_AudioDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_AudioDevice *DUMMYAUD_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = DUMMYAUD_OpenAudio;
	this->WaitAudio = DUMMYAUD_WaitAudio;
	this->PlayAudio = DUMMYAUD_PlayAudio;
	this->GetAudioBuf = DUMMYAUD_GetAudioBuf;
	this->CloseAudio = DUMMYAUD_CloseAudio;

	this->free = DUMMYAUD_DeleteDevice;

	return this;
}

AudioBootStrap DUMMYAUD_bootstrap = {
	DUMMYAUD_DRIVER_NAME, "GPlayer Audio",
	DUMMYAUD_Available, DUMMYAUD_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void DUMMYAUD_WaitAudio(_THIS)
{
	/* Don't block on first calls to simulate initial fragment filling. */
	/*if (this->hidden->initial_calls)
		this->hidden->initial_calls--;
	else
		SDL_Delay(this->hidden->write_delay ); */


#if defined (_A1)

	/* A1 Case */
	SDL_Delay(100);

#else

	Sint32 ticks;
                
	if( this->hidden->FrameTick ) 
	{				
		ticks = ( (Sint32)( this->hidden->NextFrame - SDL_GetTicks() ) ) - 10;
		if ( ticks > 0 ) {
//			printf("[DUMMYAUD_WaitAudio] ticks : %d \n", ticks);
			//SDL_Delay(ticks);
			unsigned int t = 0;
			do				
			{
				t = SDL_GetTicks();
				if( (this->hidden->NextFrame - t) < ticks/2)
				{
					SDL_Delay(ticks/10);
				}
				else
				{
					break;
				}				
			}
			while(1);
			do				
			{
				t = SDL_GetTicks();
				if( (this->hidden->NextFrame - t) < ticks)
				{
				}
				else
				{
					break;
				}				
			}
			while(1);
		} 
	}
	
#endif

}

// audio underflow : 20080131
#define	DUMMY_AUDIO_SPEED	0

#if DUMMY_AUDIO_SPEED
Uint32	g_Audio_Tick = 0;
#endif

static void DUMMYAUD_PlayAudio(_THIS)
{
#if DUMMY_AUDIO_SPEED
	Uint32	Cur_Tick = SDL_GetTicks();

	if (g_Audio_Tick != 0)
	{
		printf("\n[DUMMYAUD_PlayAudio] %d ms \tDiff = %d \n", (Cur_Tick - g_Audio_Tick), (Cur_Tick - g_Audio_Tick)-185);
	}
	g_Audio_Tick = Cur_Tick;
#endif
	
	/* no-op...this is a null driver. */
	PlayAudioBuffer( this -> hidden -> mixbuf, this -> hidden -> mixlen );
	if ( this -> hidden -> FrameTick ) { 
		//pthis -> hidden -> NextFrame = PFM_TimeNow() + this -> hidden -> FrameTick;
		this -> hidden -> NextFrame += this -> hidden -> FrameTick;
	}

}

static Uint8 *DUMMYAUD_GetAudioBuf(_THIS)
{
	return(this->hidden->mixbuf);
}

static void DUMMYAUD_CloseAudio(_THIS)
{
	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}

	AudioClose();
}

static int DUMMYAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	float bytes_per_sec = 0.0f;

	/* Allocate mixing buffer */
	this->hidden->mixlen = spec->size;

	this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);

	if ( this->hidden->mixbuf == NULL ) {
		return(-1);
	}
	SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

	bytes_per_sec = (float) (((spec->format & 0xFF) / 8) *
	                   spec->channels * spec->freq);

	/*
	 * We try to make this request more audio at the correct rate for
	 *  a given audio spec, so timing stays fairly faithful.
	 * Also, we have it not block at all for the first two calls, so
	 *  it seems like we're filling two audio fragments right out of the
	 *  gate, like other SDL drivers tend to do.
	 */
	this->hidden->initial_calls = 2;
	this->hidden->write_delay =
	               (Uint32) ((((float) spec->size) / bytes_per_sec) * 1000.0f);

	/* We're ready to rock and roll. :-) */


	//printf( " channels = %d, freq = %d \n", spec->channels, spec->freq );
	AudioInit( spec->channels, 16,  spec->freq );

	spec->samples = spec->size / ((spec->format & 0xFF) / 8);
	spec->samples /= spec->channels;
		
    	this -> hidden -> FrameTick = (float)( spec -> samples * 1000 ) / spec -> freq;
    	this -> hidden -> NextFrame = SDL_GetTicks() + this -> hidden -> FrameTick;

	//printf( "[SDL] DUMMYAUD_OpenAudio() \n" );
	
	return(0);
}

