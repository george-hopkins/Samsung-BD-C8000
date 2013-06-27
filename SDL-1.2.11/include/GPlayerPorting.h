#ifndef _GPLAYERPORTING_H_
#define _GPLAYERPORTING_H_


#include <stdio.h>
#include <stdlib.h>


//#define		USE_JOYSTICK	


#ifdef _GP_DEBUG
#define GPRINT(format, arg...)		printf("[GP Porting] %s():%d " format, __FUNCTION__, __LINE__, ##arg);
#else
#define GPRINT(format, arg...)
#endif

#define CONVERT16_LE(p)				(*(p)|(*((p)+1)<<8))
#define CONVERT32_LE(p)				(*(p)|(*((p)+1)<<8)|(*((p)+2)<<16)|(*((p)+3))<<24)

#ifdef MULTI_SHADOW

	#ifdef _16BIT_GPLAYER
		#define BEFORE_SCREEN		2
		/* jijoo 20080321 : For STi Enhancement */
		#define GPLAYER_SCREEN		3
	#else
		#define GPLAYER_SCREEN		2
	#endif
	
#else
#define GPLAYER_SCREEN		1
#endif

#define GPLAYER_SCREEN_WIDTH	960
#define GPLAYER_SCREEN_HEIGHT	540


typedef struct
{
	unsigned char name[4];
	unsigned char g_length[4];
	unsigned char formType[4];
	unsigned char fmtName[4];
	unsigned char fmtLength[4];
	unsigned char fmtTag[2];
	unsigned char channels[2];
	unsigned char Hz[4];
	unsigned char rates[4]; // rates = Hz * bytesPerSample;
	unsigned char bytesPerSample[2]; // bytesPerSample = width(bytes) * channels
	unsigned char width[2];
	unsigned char dataName[4];
	unsigned char dataLength[4];
} Riff_t; /*44bytes*/


#ifdef __cplusplus
extern	"C"	{
#endif


//===============================
// For Using SDL
//===============================

//! SDL Video Resource Init
void	VideoInit( int width, int height, int bpp );
//! Update SDL Video Surface
void	UpdateSurface(  char *buffer, int width, int height );

//! Get Frame Buffer Address for STI
char* GetFrameBufferAddress(void);

//! Close Video path for STI Bandwith Issue
void VideoClose(void);

//! SDL Audio Init
void	AudioInit( int channel, int width, int rates );
//! SDL Audio Buffer Handle
void	PlayAudioBuffer( unsigned char *buffer, unsigned int len );
//! System Audio Resource Closee
void	AudioClose( void );
//! System Audio Pause 
void AudioPause(int pause_on);


//! SDL JoyStick Device Init
int	JoyStick_Init( void );
//! SDL JoyStick Event Send
int	CheckJoyStickEvent( int *axis_event, int *axis_value, int *button_event, int *button_value  );
//! SDL JoyStick Device Close
void	JoyStick_Close( void );
//! SDL JoyStick Device Name Get
char *JoyStick_Name( int index  );



#ifdef __cplusplus
};
#endif


//! Memory Allocation in mepg context memory
void GP_MPEG_Mem_Alloc(void);
//! Memory Allocation in mepg context memory
void GP_MPEG_Mem_Free(void);


//===============================
// SDL Test Code
//===============================
void SDLPorting_Test(void);

#endif	/* _GPLAYERPORTING_H_ */


