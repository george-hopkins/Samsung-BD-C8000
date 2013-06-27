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

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include "SDL.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"

// Not used
//static unsigned short MAX_KEY_CODE;

static SDLKey  keymap[ 256 ];

extern	int	InputKeyData, InputKeyState;

static SDL_keysym *TranslateKey( unsigned short vkey, SDL_keysym *keysym )
{
	/* Set the keysym information */
	keysym->scancode = vkey;
	keysym->sym = keymap[ vkey ];
	keysym->mod = KMOD_NONE;
	keysym->unicode = 0;
		
	return(keysym);
}

void DUMMY_PumpEvents(_THIS)
{
	SDL_keysym keysym;
	
	if( InputKeyState == 1 ) {
		//printf( "[SDL - %s] KEY_PRESSED vvvvvvv \n\n", __FUNCTION__ );	
		SDL_PrivateKeyboard( SDL_PRESSED, TranslateKey( InputKeyData, &keysym ) );

#ifdef STI_Ready
		//InputKeyState = 2;
		SDL_PrivateKeyboard( SDL_RELEASED, TranslateKey( InputKeyData, &keysym ) );
#endif

	}	
	if( InputKeyState == 2 ) {
		//printf( "[SDL - %s]  KEY_RELEASED ~~~~~ \n\n", __FUNCTION__); 
		SDL_PrivateKeyboard( SDL_RELEASED, TranslateKey( InputKeyData, &keysym ) );
	}
								
	InputKeyState = 0;
}

void DUMMY_InitOSKeymap(_THIS)
{
	keymap[ dKEY_RETURN ] 			= SDL_QUIT ;

	keymap[ dKEY_0 ] 				= SDLK_0 ;			/*	Number Key	*/
	keymap[ dKEY_1 ] 				= SDLK_1 ;			/*	Number Key	*/
	keymap[ dKEY_2 ] 				= SDLK_2 ;			/*	Number Key	*/
	keymap[ dKEY_3 ] 				= SDLK_3 ;			/*	Number Key	*/
	keymap[ dKEY_4 ] 				= SDLK_4 ;			/*	Number Key	*/

	keymap[ dKEY_5 ] 				= SDLK_5 ;			/*	Number Key	*/
	keymap[ dKEY_6 ] 				= SDLK_6 ;			/*	Number Key	*/
	keymap[ dKEY_7 ] 				= SDLK_7 ;			/*	Number Key	*/
	keymap[ dKEY_8 ] 				= SDLK_8 ;			/*	Number Key	*/
	keymap[ dKEY_9 ] 				= SDLK_9 ;			/*	Number Key	*/


	keymap[ dKEY_JOYSTICK_OK ] 		= SDLK_z ;			/*	Action	*/
	keymap[ dKEY_JOYSTICK_UP ] 		= SDLK_UP ;			/*	Up		*/
	keymap[ dKEY_JOYSTICK_DOWN ] 	= SDLK_DOWN ;		/*	Down	*/
	keymap[ dKEY_JOYSTICK_LEFT ] 	= SDLK_LEFT ;		/*	Left		*/
	keymap[ dKEY_JOYSTICK_RIGHT ] 	= SDLK_RIGHT ;		/*	Right	*/

	keymap[ dKEY_RED ] 				= SDLK_HOME ;		/*	Home 	*/ 
	keymap[ dKEY_GREEN ]			= SDLK_F1 ;			/*	Option1 	*/
	keymap[ dKEY_YELLOW ] 			= SDLK_F2 ;			/*	Option2 	*/
	keymap[ dKEY_BLUE ] 				= SDLK_ESCAPE ;		/*	Exit	 	*/

	keymap[ dKEY_EXIT ] 				= SDLK_POWER ;		/*	Direct Exit	*/
	keymap[ dKEY_RESERVED7 ]			= SDLK_F13 ;		/*	InfoLink	*/
	keymap[ dKEY_ZOOM1 ]			= SDLK_F13 ;		/*	InfoLink	*/

	keymap[ dKEY_MUTE ]		= SDLK_F10 ;			/* MUTE */
	keymap[ dKEY_VOLUP ] 		= SDLK_F11 ;			/*	Volume Up	7*/
	keymap[ dKEY_VOLDOWN ] 		= SDLK_F12 ;			/*	Volume Down	11	*/
	
	keymap[ dKEY_PLAY ] 	= SDLK_F3 ;		/*	Play	*/
	keymap[ dKEY_PAUSE ] 	= SDLK_F4 ;		/*	Pause	*/
	keymap[ dKEY_STOP ] 	= SDLK_F5 ;		/*	Stop	*/

	keymap[ dKEY_REWIND ] 				= SDLK_F6 ;		/*	Rew 	*/ 
	keymap[ dKEY_FF ]			= SDLK_F7 ;			/*	FF 	*/


	/* etc..... */	
}

/* end of SDL_nullevents.c ... */

