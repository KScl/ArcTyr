/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  vga256d.c
/// \brief TODO

#include "config.h" // For fullscreen stuff
#include "opentyr.h"
#include "palette.h"
#include "vga256d.h"
#include "video.h"

#include "SDL.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void JE_pix( SDL_Surface *surface, int x, int y, JE_byte c )
{
	/* Bad things happen if we don't clip */
	if (x <  surface->pitch && y <  surface->h)
	{
		Uint8 *vga = surface->pixels;
		vga[y * surface->pitch + x] = c;
	}
}

void JE_pix3( SDL_Surface *surface, int x, int y, JE_byte c )
{
	/* Originally impemented as several direct accesses */
	JE_pix(surface, x, y, c);
	JE_pix(surface, x - 1, y, c);
	JE_pix(surface, x + 1, y, c);
	JE_pix(surface, x, y - 1, c);
	JE_pix(surface, x, y + 1, c);
}

void JE_rectangle( SDL_Surface *surface, int a, int b, int c, int d, int e ) /* x1, y1, x2, y2, color */
{
	if (a < surface->pitch && b < surface->h &&
	    c < surface->pitch && d < surface->h)
	{
		Uint8 *vga = surface->pixels;
		int i;

		/* Top line */
		memset(&vga[b * surface->pitch + a], e, c - a + 1);

		/* Bottom line */
		memset(&vga[d * surface->pitch + a], e, c - a + 1);

		/* Left line */
		for (i = (b + 1) * surface->pitch + a; i < (d * surface->pitch + a); i += surface->pitch)
		{
			vga[i] = e;
		}

		/* Right line */
		for (i = (b + 1) * surface->pitch + c; i < (d * surface->pitch + c); i += surface->pitch)
		{
			vga[i] = e;
		}
	} else {
		printf("!!! WARNING: Rectangle clipped: %d %d %d %d %d\n", a, b, c, d, e);
	}
}

void fill_rectangle_xy( SDL_Surface *surface, int x, int y, int x2, int y2, Uint8 color )
{
	SDL_Rect rect = { x, y, x2 - x + 1, y2 - y + 1 };
	SDL_FillRect(surface, &rect, color);
}

void JE_barShade( SDL_Surface *surface, int a, int b, int c, int d ) /* x1, y1, x2, y2 */
{
	if (a < surface->pitch && b < surface->h &&
	    c < surface->pitch && d < surface->h)
	{
		Uint8 *vga = surface->pixels;
		int i, j, width;

		width = c - a + 1;

		for (i = b * surface->pitch + a; i <= d * surface->pitch + a; i += surface->pitch)
		{
			for (j = 0; j < width; j++)
			{
				vga[i + j] = ((vga[i + j] & 0x0F) >> 1) | (vga[i + j] & 0xF0);
			}
		}
	} else {
		printf("!!! WARNING: Darker Rectangle clipped: %d %d %d %d\n", a,b,c,d);
	}
}

void JE_barBright( SDL_Surface *surface, int a, int b, int c, int d ) /* x1, y1, x2, y2 */
{
	if (a < surface->pitch && b < surface->h &&
	    c < surface->pitch && d < surface->h)
	{
		Uint8 *vga = surface->pixels;
		int i, j, width;

		width = c-a+1;

		for (i = b * surface->pitch + a; i <= d * surface->pitch + a; i += surface->pitch)
		{
			for (j = 0; j < width; j++)
			{
				JE_byte al, ah;
				al = ah = vga[i + j];

				ah &= 0xF0;
				al = (al & 0x0F) + 2;

				if (al > 0x0F)
				{
					al = 0x0F;
				}

				vga[i + j] = al + ah;
			}
		}
	} else {
		printf("!!! WARNING: Brighter Rectangle clipped: %d %d %d %d\n", a,b,c,d);
	}
}

void draw_segmented_gauge( SDL_Surface *surface, int x, int y, Uint8 color, uint segment_width, uint segment_height, uint segment_value, uint value )
{
	assert(segment_width > 0 && segment_height > 0);

	const uint segments = value / segment_value,
	           partial_segment = value % segment_value;

	for (uint i = 0; i < segments; ++i)
	{
		fill_rectangle_hw(surface, x, y, segment_width, segment_height, color + 12);
		x += segment_width + 1;
	}
	if (partial_segment > 0)
		fill_rectangle_hw(surface, x, y, segment_width, segment_height, color + (12 * partial_segment / segment_value));
}

