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

#if defined(SDL_JOYSTICK_DUMMY) || defined(SDL_JOYSTICK_DISABLED)

/* This is the system specific header for the SDL joystick API */

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */


#define	MAX_JOYSTICK_BUTTON	16

extern	int	JoyStick_Init( void );
extern	int	CheckJoyStickEvent( int *axis_event, int *axis_value, int *button_event, int *button_value );
extern	void	JoyStick_Close( void );
extern	char *JoyStick_Name( int index  );


static int	JoystickPress[ MAX_JOYSTICK_BUTTON ];

// Not used
//static unsigned int	Joystick_Tick, Joystick_NextTick;


struct joystick_hwdata
{
	unsigned long joystate;
};


int SDL_SYS_JoystickInit(void)
{
	int i =0;

	SDL_numjoysticks = JoyStick_Init();

	for( i = 0; i < MAX_JOYSTICK_BUTTON; i++ ) JoystickPress[ i ] = 0;  

	return SDL_numjoysticks;
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{

	return	JoyStick_Name( index );
		
	//SDL_SetError("Logic error: No joysticks available");
	//return(NULL);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));

    	joystick->nbuttons = MAX_JOYSTICK_BUTTON;
	joystick->nhats = 0;
	joystick->nballs = 0;
	joystick->naxes = 2;
	joystick->hwdata->joystate = 0L;
	
	//SDL_SetError("Logic error: No joysticks available");
	
	return (int)(NULL);
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	int	axis_event = -1, axis_value = -1, button_event = 0, button_value = -1;
	int	button = 0;
	int i = 0;
		
	CheckJoyStickEvent( &axis_event, &axis_value, &button_event, &button_value );

	
 	// x축 이벤트 들어옴 
	if( axis_event == 0  ) {
		SDL_PrivateJoystickAxis( joystick, 0, axis_value );
	}
	// Y 축 이벤트 들어옴.
	if( axis_event == 1 ) {
		SDL_PrivateJoystickAxis( joystick, 1, axis_value );
	}	
		
	// JoyStick Button 처리
	if( button_value == 1 )
	{
	#if 0
		button = button_event;
		if( button >= BTN_MISC && button <= BTN_9 ) {
			button -= BTN_MISC;
		}
		else
		if( button >= BTN_JOYSTICK && button <= BTN_DEAD ) {
			button -= BTN_JOYSTICK;
		}
		else
		if( button >= BTN_GAMEPAD && button <= BTN_THUMBR ) {
			button -= BTN_GAMEPAD;
		}
	#else
		button = button_event;
		if( button >= 0x100 && button <= 0x109 ) {
			button -= 0x100;
		}
		else
		if( button >= 0x120 && button <= 0x12f ) {
			button -= 0x120;
		}
		else
		if( button >= 0x130 && button <= 0x13e ) {
			button -= 0x130;
		}
	#endif
	
		printf("[SDL_SYS_JoystickUpdate] button_event 0x%x , button_index %d \n", button_event, button);

		if ( (button>= 0) && (button < MAX_JOYSTICK_BUTTON) )
		{
			if(  JoystickPress[ button ] == 0 ) {
				JoystickPress[ button ] = 1;
				//SDL_PrivateJoystickButton( joystick, button, SDL_PRESSED ); 
				SDL_PrivateJoystickButton( joystick, button, 1 );
			}
		}
		
	}

	if( button_value == 0 ) {
		for( i = 0; i < joystick->nbuttons; i++ ) {
			if( JoystickPress[ i ] ) {
				JoystickPress[ i ] = 0;
				//SDL_PrivateJoystickButton(joystick,i,SDL_RELEASED);
				SDL_PrivateJoystickButton(joystick,i,0);
			}
		}
	}
	
	return;
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if(joystick->hwdata)
		free(joystick->hwdata);
	
	return;
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	SDL_numjoysticks=0;
	JoyStick_Close();
	
	return;
}

#endif /* SDL_JOYSTICK_DUMMY || SDL_JOYSTICK_DISABLED */
