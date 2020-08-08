/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  video/scaler.c
/// \brief Generic scaling handler

#include "scaler.h"

#include "../palette.h"
#include "../video.h"

#include <assert.h>

static void no_scale( SDL_Surface *src_surface, SDL_Surface *dst_surface );
static void nn_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
static void nn_16( SDL_Surface *src_surface, SDL_Surface *dst_surface );

uint scaler;

const struct Scalers scalers[] =
{
#if defined(TARGET_GP2X) || defined(TARGET_DINGUX)
	{ 320,           240,            no_scale, nn_16,      nn_32,      "None" },
#else

	{ 1 * vga_width, 1 * vga_height, no_scale, nn_16,      nn_32,      "None" },
	{ 2 * vga_width, 2 * vga_height, NULL,     nn_16,      nn_32,      "2x" },
	{ 3 * vga_width, 3 * vga_height, NULL,     nn_16,      nn_32,      "3x" },
	{ 4 * vga_width, 4 * vga_height, NULL,     nn_16,      nn_32,      "4x" },

#ifdef ENABLE_SCALENX_SCALER
	{ 2 * vga_width, 2 * vga_height, NULL,     scale2x_16, scale2x_32, "Scale2x" },
	{ 3 * vga_width, 3 * vga_height, NULL,     scale3x_16, scale3x_32, "Scale3x" },
#endif

#ifdef ENABLE_HQNX_SCALER
	{ 2 * vga_width, 2 * vga_height, NULL,     NULL,       hq2x_32,    "hq2x" },
	{ 3 * vga_width, 3 * vga_height, NULL,     NULL,       hq3x_32,    "hq3x" },
	{ 4 * vga_width, 4 * vga_height, NULL,     NULL,       hq4x_32,    "hq4x" },
#endif

#endif
};
const uint scalers_count = COUNTOF(scalers);

void set_scaler_by_name( const char *name )
{
	for (uint i = 0; i < scalers_count; ++i)
	{
		if (strcmp(name, scalers[i].name) == 0)
		{
			scaler = i;
			break;
		}
	}
}

#if defined(TARGET_GP2X) || defined(TARGET_DINGUX)
#define VGA_CENTERED
#endif

void no_scale( SDL_Surface *src_surface, SDL_Surface *dst_surface )
{
	Uint8 *src = src_surface->pixels,
	      *dst = dst_surface->pixels;
	
#ifdef VGA_CENTERED
	size_t blank = (dst_surface->h - src_surface->h) / 2 * dst_surface->pitch;
	memset(dst, 0, blank);
	dst += blank;
#endif
	
	memcpy(dst, src, src_surface->pitch * src_surface->h);
	
#ifdef VGA_CENTERED
	dst += src_surface->pitch * src_surface->h;
	memset(dst, 0, blank);
#endif
}


void nn_32( SDL_Surface *src_surface, SDL_Surface *dst_surface )
{
	Uint8 *src = src_surface->pixels, *src_temp,
	      *dst = dst_surface->pixels, *dst_temp;
	int src_pitch = src_surface->pitch,
	    dst_pitch = dst_surface->pitch;
	const int dst_Bpp = 4;         // dst_surface->format->BytesPerPixel
	
	const int height = vga_height, // src_surface->h
	          width = vga_width,   // src_surface->w
	          scale = dst_surface->w / width;
	assert(scale == dst_surface->h / height);
	
#ifdef VGA_CENTERED
	size_t blank = (dst_surface->h - src_surface->h) / 2 * dst_surface->pitch;
	memset(dst, 0, blank);
	dst += blank;
#endif
	
	for (int y = height; y > 0; y--)
	{
		src_temp = src;
		dst_temp = dst;
		
		for (int x = width; x > 0; x--)
		{
			for (int z = scale; z > 0; z--)
			{
				*(Uint32 *)dst = rgb_palette[*src];
				dst += dst_Bpp;
			}
			src++;
		}
		
		src = src_temp + src_pitch;
		dst = dst_temp + dst_pitch;
		
		for (int z = scale; z > 1; z--)
		{
			memcpy(dst, dst_temp, dst_pitch);
			dst += dst_pitch;
		}
	}
	
#ifdef VGA_CENTERED
	memset(dst, 0, blank);
#endif
}

void nn_16( SDL_Surface *src_surface, SDL_Surface *dst_surface )
{
	Uint8 *src = src_surface->pixels, *src_temp,
	      *dst = dst_surface->pixels, *dst_temp;
	int src_pitch = src_surface->pitch,
	    dst_pitch = dst_surface->pitch;
	const int dst_Bpp = 2;         // dst_surface->format->BytesPerPixel
	
	const int height = vga_height, // src_surface->h
	          width = vga_width,   // src_surface->w
	          scale = dst_surface->w / width;
	assert(scale == dst_surface->h / height);
	
#ifdef VGA_CENTERED
	size_t blank = (dst_surface->h - src_surface->h) / 2 * dst_surface->pitch;
	memset(dst, 0, blank);
	dst += blank;
#endif
	
	for (int y = height; y > 0; y--)
	{
		src_temp = src;
		dst_temp = dst;
		
		for (int x = width; x > 0; x--)
		{
			for (int z = scale; z > 0; z--)
			{
				*(Uint16 *)dst = rgb_palette[*src];
				dst += dst_Bpp;
			}
			src++;
		}
		
		src = src_temp + src_pitch;
		dst = dst_temp + dst_pitch;
		
		for (int z = scale; z > 1; z--)
		{
			memcpy(dst, dst_temp, dst_pitch);
			dst += dst_pitch;
		}
	}
	
#ifdef VGA_CENTERED
	memset(dst, 0, blank);
#endif
}
