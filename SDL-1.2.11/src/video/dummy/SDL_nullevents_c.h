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

#include "SDL_nullvideo.h"

/* Variables and functions exported by SDL_sysevents.c to other parts 
   of the native video subsystem (SDL_sysvideo.c)
*/
extern void DUMMY_InitOSKeymap(_THIS);
extern void DUMMY_PumpEvents(_THIS);


/* Shadow에서 사용하게 될 리모턴 키 */
enum KEY_VALUE_DEFINE
{
	dKEY_RETURN			= 88,

	dKEY_0				= 17,
	dKEY_1				= 101,		// wayforu - convert
	dKEY_2				= 98,		// wayforu - convert
	dKEY_3				= 6,
	dKEY_4				= 8,
	
	dKEY_5				= 9,
	dKEY_6				= 10,
	dKEY_7				= 12,
	dKEY_8				= 13,
	dKEY_9				= 14,

	dKEY_JOYSTICK_OK	= 0,
	dKEY_JOYSTICK_UP	= 2,
	dKEY_JOYSTICK_DOWN	= 3,
	dKEY_JOYSTICK_LEFT	= 4,
	dKEY_JOYSTICK_RIGHT	= 5,

	dKEY_RED			= 108,
	dKEY_GREEN			= 20,
	dKEY_YELLOW		    = 21,
	dKEY_BLUE			= 22,

	dKEY_EXIT			= 45,
	dKEY_RESERVED7		= 147,
	dKEY_ZOOM1			= 83,		// wayforu11

	dKEY_MUTE			= 15,
	dKEY_VOLUP			= 7,
	dKEY_VOLDOWN		= 11,

	dKEY_PLAY			= 71,		// |▶
	dKEY_PAUSE			= 74,		// ||
	dKEY_STOP			= 70,		// ■

	dKEY_REWIND			= 69,		// ◀◀
	dKEY_FF				= 72,		// ▶▶

#if 0 // not used
	dKEY_MENU				= 1,
	dKEY_CHDOWN			= 16,
	dKEY_CHUP			= 18,
	dKEY_PRECH			= 19,

//Color Key 정의
	//dKEY_TTX_GREEN		= 20,
	//dKEY_TTX_YELLOW		= 21,
	//dKEY_TTX_CYAN		= 22,

	dKEY_STEP			= 23,		// TVDP
	dKEY_DEL				= 24,
	dKEY_ADDDEL			= 25,		// ADD/DEL
	dKEY_SOURCE			= 26,		// wayforu - convert  // (TV/VIDEO) 
	dKEY_TV				= 27,		// wayforu11
	dKEY_AUTO			= 28,
	dKEY_DOUBLESCREEN	= 29,
	dKEY_PMENU			= 30,
	dKEY_DISPLAY			= 31,		// INFO
	dKEY_PIP_ONOFF		= 32,
	dKEY_PIP_SWAP		= 33,
	dKEY_PIP_ROTATE		= 34,
	dKEY_PLUS100			= 35,
	dKEY_PIP_INPUT		= 36,
	dKEY_CAPTION			= 37,
	dKEY_PIP_STILL		= 38,
	dKEY_DSP				= 39,
	dKEY_PMODE			= 40,
	dKEY_SOUND_MODE		= 41,
	dKEY_NR				= 42,		// N/R
	dKEY_SMODE			= 43,
	dKEY_TTX_MIX			= 44,		// TTX/MIX
	dKEY_ENTER			= 46,
	dKEY_PIP_SIZE		= 47,
	dKEY_MAGIC_CHANNEL	= 48,
	dKEY_PIP_SCAN		= 49,		// SEARCH
	dKEY_PIP_CHUP		= 50,
	dKEY_PIP_CHDOWN		= 51,
	dKEY_DVD_SETUP		= 52,
	dKEY_HELP			= 53,		// CH INFO
	dKEY_ANTENA			= 54,
	dKEY_CONVERGENCE		= 55,
	dKEY_11				= 56,
	dKEY_12				= 57,
	dKEY_AUTO_PROGRAM	= 58,
	dKEY_FACTORY			= 59,
	dKEY_3SPEED			= 60,
	dKEY_RSURF			= 61,
	dKEY_ASPECT			= 62,		// P.SIZE
	dKEY_TOPMENU			= 63,
	dKEY_GAME			= 64,
	dKEY_QUICK_REPLAY	= 65,
	dKEY_STILL_PICTURE	= 66,		// FREEZE
	dKEY_DTV				= 67,
	dKEY_FAVCH			= 68,		// CH.LIST
	dKEY_REC				= 73,		// ●
	dKEY_MYLIST			= 75,
	dKEY_INSTANT_REPLAY	= 76,
	dKEY_LINK			= 77,		// MSG, IEEE-1394 D.NET
	dKEY_FF_				= 78,		// ▶▶|(TVDP)
	dKEY_GUIDE			= 79,
	dKEY_REWIND_			= 80,		// |◀◀(TVDP)
	dKEY_ANGLE			= 81,		// MULTI SCREEN
	dKEY_RESERVED1		= 82,
	dKEY_PROGRAM			= 84,		// INSTALL,
	dKEY_BOOKMARK		= 85,
	dKEY_DISC_MENU		= 86,
	dKEY_TITLE_MENU		= 87,		// QUICK
	dKEY_SUB_TITLE		= 89,
	dKEY_CLEAR			= 90,
	dKEY_VCHIP			= 91,
	dKEY_REPEAT			= 92,
	dKEY_DOOR			= 93,
	dKEY_OPEN			= 94,		// CLOSE
	dKEY_NOCODE			= 95,
	dKEY_POWER			= 96,		// wayforu - convert
	dKEY_SLEEP			= 97,		// wayforu - convert
	dKEY_INPUT			= 99,
	dKEY_TURBO			= 100,
	dKEY_FM_RADIO		= 102,
	dKEY_DVR_MENU		= 103,
	dKEY_MTS				= 104,	 	// wayforu - convert	// 음성 다중	
	dKEY_PCMODE			= 105,		// TV DVD
	dKEY_TTX_SUBFACE		= 106,
	dKEY_LIST			= 107,		// CH LIST

	//dKEY_TTX_RED			= 108,
	
	dKEY_DNIe			= 109,
	dKEY_DOLBY_SURROUND	= 110,		// SRS
	dKEY_CONVERT_AUDIO_MAINSUB	= 111,	// 음향주부전환
	dKEY_MDC				= 112,
	dKEY_SEFFECT			= 113,
	dKEY_DVR				= 114,
	dKEY_DTV_SIGNAL		= 115,		// 신호세기
	dKEY_LIVE			= 116,		// EQ
	dKEY_PERPECT_FOCUS	= 117,		// 자동조정(AUTO), RESOLUTION
	dKEY_HOME			= 118,
	dKEY_LOCK			= 119,		// PWR.SAVE
	dKEY_RESERVED3		= 120,
	dKEY_RESERVED4		= 121,
	dKEY_VCR_MODE		= 122,
	dKEY_CATV_MODE		= 123,
	dKEY_DSS_MODE		= 124,
	dKEY_TV_MODE			= 125,
	dKEY_DVD_MODE		= 126,
	dKEY_STB_MODE		= 127,
	dKEY_CALLER_ID		= 128,
	dKEY_SCALE			= 129,		// V.KEYSTONE
	dKEY_ZOOM_MOVE		= 130,		// 화면확대/이동
	dKEY_CLOCK_DISPLAY	= 131,
	dKEY_AV1				= 132,		// wayforu11
	dKEY_SVIDEO1			= 133,		// wayforu11
	dKEY_COMPONENT1		= 134,		// wayforu11
	dKEY_SETUP_CLOCK_TIMER = 135, 
	dKEY_COMPONENT2		= 136,		// wayforu11
	dKEY_MAGIC_BRIGHT	= 137,
	dKEY_DVI				= 138,
	dKEY_HDMI			= 139,		// wayforu11
	dKEY_W_LINK			= 140,		// MEMORY CARD
	dKEY_DTV_LINK		= 141,
	dKEY_RESERVED5		= 142,
	dKEY_APP_MHP			= 143,		// MHP
	dKEY_BACK_MHP		= 144,		// MHP
	dKEY_ALT_MHP			= 145,		// MHP
	dKEY_DNSe			= 146,		// DNSE
	dKEY_RESERVED8		= 148,
	dKEY_ID_INPUT		= 149,
	dKEY_ID_SETUP		= 150,
	dKEY_ANYNET			= 151,
	dKEY_POWEROFF		= 152,	
	dKEY_POWERON			= 153,	
	dKEY_ANYVIEW			= 154,
	dKEY_PANNEL_POWER	= 155,
	dKEY_PANNEL_CHUP		= 156,
	dKEY_PANNEL_CHDOWN	= 157,
	dKEY_PANNEL_VOLUP	= 158,
	dKEY_PANNEL_VOLDOWN	= 159,	
	dKEY_PANNEL_ENTER	= 160,
	dKEY_PANNEL_MENU		= 161,
	dKEY_PANNEL_SOURCE	= 162,	
	dKEY_AV2				= 163,
	dKEY_AV3				= 164,
	dKEY_SVIDEO2			= 165,
	dKEY_SVIDEO3			= 166,
	dKEY_ZOOM2			= 167,
	dKEY_PANORAMA		= 168,
	dKEY_4_3				= 169,
	dKEY_16_9			= 170,
	dKEY_DYNAMIC			= 171,
	dKEY_STANDARD		= 172,
	dKEY_MOVIE1			= 173,
	dKEY_CUSTOM			= 174,

	dKEY_AUTO_ARC_RESET	= 175,
	dKEY_AUTO_ARC_LNA_ON	= 176,
	dKEY_AUTO_ARC_LNA_OFF			= 177,
	dKEY_AUTO_ARC_ANYNET_MODE_OK		= 178,
	dKEY_AUTO_ARC_ANYNET_AUTO_START	= 179,

	dKEY_AUTO_FORMAT 		= 180,
	dKEY_DNET			= 181,
	dKEY_HDMI1			= 182, 

	dKEY_AUTO_ARC_CAPTION_ON		= 183,
	dKEY_AUTO_ARC_CAPTION_OFF	= 184,

	dKEY_AUTO_ARC_PIP_DOUBLE		= 185,
	dKEY_AUTO_ARC_PIP_LARGE		= 186,
	dKEY_AUTO_ARC_PIP_SMALL		= 187,
	dKEY_AUTO_ARC_PIP_WIDE		= 188,
	dKEY_AUTO_ARC_PIP_LEFT_TOP		= 189,
	dKEY_AUTO_ARC_PIP_RIGHT_TOP		= 190,
	dKEY_AUTO_ARC_PIP_LEFT_BOTTOM	= 191,
	dKEY_AUTO_ARC_PIP_RIGHT_BOTTOM	= 192,
	dKEY_AUTO_ARC_PIP_CH_CHANGE  = 193,
	dKEY_AUTO_ARC_AUTOCOLOR_SUCCESS = 194,
	dKEY_AUTO_ARC_AUTOCOLOR_FAIL = 195,
	dKEY_AUTO_ARC_C_FORCE_AGING = 196,
	dKEY_AUTO_ARC_USBJACK_INSPECT = 197,
	dKEY_AUTO_ARC_JACK_IDENT = 198,
	
	dKEY_NINE_SEPERATE = 199,
	dKEY_ZOOM_IN = 200,
	dKEY_ZOOM_OUT = 201,
	dKEY_MIC = 202,	

	dKEY_HDMI2 = 203,  // discrete key
	dKEY_HDMI3 =204,  // discrete key
#endif

	dKEY_MAX				 		= 205
};

/* end of SDL_nullevents_c.h ... */

