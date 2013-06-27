#ifndef __YAHOO_SDL_HW_ACCELERATION_H__
#define __YAHOO_SDL_HW_ACCELERATION_H__


#define SDL_SCREEN		1

#define GPLAYER_SCREEN_WIDTH	960
#define GPLAYER_SCREEN_HEIGHT	540


#ifdef __cplusplus
extern "C" {
#endif

int y_HWFillRect(SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
int y_HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int y_HWSWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int y_SetHWColorKey(SDL_Surface* surface, Uint32 key);
int y_SetHWAlpha(SDL_Surface* surface, Uint8 value);
int y_AllocHWSurface(SDL_Surface *surface);
void y_FreeHWSurface(SDL_Surface *surface);

#ifdef __cplusplus
};
#endif

#endif	// __YAHOO_SDL_HW_ACCELERATION_H__
