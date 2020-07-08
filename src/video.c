/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "arcade.h"
#include "input.h"
#include "opentyr.h"
#include "palette.h"
#include "video.h"
#include "video/scaler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

bool fullscreen_enabled = false;

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

static ScalerFunction scaler_function;

bool skip_header_draw = false;
bool skip_header_undraw = false;
static void *header_cache_buf = NULL;
static size_t header_cache_size = 0;
static size_t header_cache_offset = 0;

static void draw_arc_header( SDL_Surface *src_surface )
{
	if (skip_header_draw)
		return;

	if (!header_cache_buf)
	{
		header_cache_offset = src_surface->pitch * 1;
		header_cache_size = src_surface->pitch * 28;
		header_cache_buf = malloc(header_cache_size);
		if (!header_cache_buf)
		{
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	if (!skip_header_undraw)
		memcpy(header_cache_buf, (unsigned char*)src_surface->pixels + header_cache_offset, header_cache_size);

	if (play_demo)
	{
		ARC_DISP_NoPlayerInSlot(1);
		ARC_DISP_NoPlayerInSlot(2);
	}
	else if (!isInGame)
	{
		if (player[0].player_status == STATUS_NONE)
			ARC_DISP_NoPlayerInSlot(1);
		if (player[1].player_status == STATUS_NONE)
			ARC_DISP_NoPlayerInSlot(2);
	}
	else for (int p = 1; p <= 2; ++p)
	{
		switch (player[p-1].player_status)
		{
			default:
			case STATUS_NONE:      ARC_DISP_NoPlayerInSlot(p); break;
			case STATUS_INGAME:    ARC_DISP_InGameDisplay(p);  break;
			case STATUS_NAMEENTRY: ARC_DISP_HighScoreEntry(p); break;
			case STATUS_CONTINUE:  ARC_DISP_ContinueEntry(p);  break;
			case STATUS_SELECT:    ARC_DISP_MidGameSelect(p);  break;
			case STATUS_GAMEOVER:  ARC_DISP_GameOverHold(p);   break;
		}
	}
}

static void undraw_arc_header( SDL_Surface *src_surface )
{
	if (skip_header_draw || skip_header_undraw)
		return;

	memcpy((unsigned char*)src_surface->pixels + header_cache_offset, header_cache_buf, header_cache_size);
}

void init_video( void )
{
	if (SDL_WasInit(SDL_INIT_VIDEO))
		return;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		fprintf(stderr, "error: failed to initialize SDL video: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("ArcTyr", NULL);

	VGAScreen = VGAScreenSeg = SDL_CreateRGBSurface(SDL_SWSURFACE, vga_width, vga_height, 8, 0, 0, 0, 0);
	VGAScreen2 = SDL_CreateRGBSurface(SDL_SWSURFACE, vga_width, vga_height, 8, 0, 0, 0, 0);
	game_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, vga_width, vga_height, 8, 0, 0, 0, 0);

	SDL_FillRect(VGAScreen, NULL, 0);

	if (!init_scaler(scaler, fullscreen_enabled) &&  // try desired scaler and desired fullscreen state
	    !init_any_scaler(fullscreen_enabled) &&      // try any scaler in desired fullscreen state
	    !init_any_scaler(!fullscreen_enabled))       // try any scaler in other fullscreen state
	{
		fprintf(stderr, "error: failed to initialize any supported video mode\n");
		exit(EXIT_FAILURE);
	}
}

int can_init_scaler( unsigned int new_scaler, bool fullscreen )
{
	if (new_scaler >= scalers_count)
		return false;
	
	int w = scalers[new_scaler].width,
	    h = scalers[new_scaler].height;
	int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);
	
	// test each bitdepth
	for (uint bpp = 32; bpp > 0; bpp -= 8)
	{
		uint temp_bpp = SDL_VideoModeOK(w, h, bpp, flags);
		
		if ((temp_bpp == 32 && scalers[new_scaler].scaler32) ||
		    (temp_bpp == 16 && scalers[new_scaler].scaler16) ||
		    (temp_bpp == 8  && scalers[new_scaler].scaler8 ))
		{
			return temp_bpp;
		}
		else if (temp_bpp == 24 && scalers[new_scaler].scaler32)
		{
			// scalers don't support 24 bpp because it's a pain
			// so let SDL handle the conversion
			return 32;
		}
	}
	
	return 0;
}

bool init_scaler( unsigned int new_scaler, bool fullscreen )
{
	int w = scalers[new_scaler].width,
	    h = scalers[new_scaler].height;
	int bpp = can_init_scaler(new_scaler, fullscreen);
	int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);
	
	if (bpp == 0)
		return false;
	
	SDL_Surface *const surface = SDL_SetVideoMode(w, h, bpp, flags);
	
	if (surface == NULL)
	{
		fprintf(stderr, "error: failed to initialize %s video mode %dx%dx%d: %s\n", fullscreen ? "fullscreen" : "windowed", w, h, bpp, SDL_GetError());
		return false;
	}
	
	w = surface->w;
	h = surface->h;
	bpp = surface->format->BitsPerPixel;
	
	printf("initialized video: %dx%dx%d %s\n", w, h, bpp, fullscreen ? "fullscreen" : "windowed");
	
	scaler = new_scaler;
	fullscreen_enabled = fullscreen;
	
	switch (bpp)
	{
	case 32:
		scaler_function = scalers[scaler].scaler32;
		break;
	case 16:
		scaler_function = scalers[scaler].scaler16;
		break;
	case 8:
		scaler_function = scalers[scaler].scaler8;
		break;
	default:
		scaler_function = NULL;
		break;
	}
	
	if (scaler_function == NULL)
	{
		assert(false);
		return false;
	}
	
	JE_showVGA();
	
	return true;
}

bool can_init_any_scaler( bool fullscreen )
{
	for (int i = scalers_count - 1; i >= 0; --i)
		if (can_init_scaler(i, fullscreen) != 0)
			return true;
	
	return false;
}

bool init_any_scaler( bool fullscreen )
{
	// attempts all scalers from last to first
	for (int i = scalers_count - 1; i >= 0; --i)
		if (init_scaler(i, fullscreen))
			return true;
	
	return false;
}

void deinit_video( void )
{
	SDL_FreeSurface(VGAScreenSeg);
	SDL_FreeSurface(VGAScreen2);
	SDL_FreeSurface(game_screen);
	
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	if (header_cache_buf)
		free(header_cache_buf);
}

void JE_clr256( SDL_Surface * screen)
{
	memset(screen->pixels, 0, screen->pitch * screen->h);
}
void JE_showVGA( void ) { scale_and_flip(VGAScreen); }

uint nextSec = 1000, curSec, fps;
// don't use tmpBuf here
char fpsTexBuf[64] = "";

void scale_and_flip( SDL_Surface *src_surface )
{
	assert(src_surface->format->BitsPerPixel == 8);
	
	SDL_Surface *dst_surface = SDL_GetVideoSurface();

	assert(scaler_function != NULL);

	draw_arc_header(src_surface);
	scaler_function(src_surface, dst_surface);
	undraw_arc_header(src_surface);
	
	SDL_Flip(dst_surface);

	++fps;
	if ((curSec = SDL_GetTicks()) >= nextSec)
	{
		if (curSec > 10000)
		{
			sprintf(fpsTexBuf, "ArcTyr [FPS: %d]", fps);
			SDL_WM_SetCaption(fpsTexBuf, NULL);
		}

		fps = 0;
		do { nextSec += 1000; } while (curSec >= nextSec);
	}
}

