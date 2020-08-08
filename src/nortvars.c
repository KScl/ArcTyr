/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  nortvars.c
/// \brief TODO These functions are unsorted

#include "file.h"
#include "input.h"
#include "nortvars.h"
#include "opentyr.h"
#include "vga256d.h"
#include "video.h"

#include <assert.h>
#include <ctype.h>

void JE_dBar3( SDL_Surface *surface, JE_integer x,  JE_integer y,  JE_integer num,  JE_integer col )
{
	JE_byte z;
	JE_byte zWait = 2;

	col += 2;

	for (z = 0; z <= num; z++)
	{
		JE_rectangle(surface, x, y - 1, x + 8, y, col); /* <MXD> SEGa000 */
		if (zWait > 0)
		{
			zWait--;
		} else {
			col++;
			zWait = 1;
		}
		y -= 2;
	}
}

void JE_barDrawShadow( SDL_Surface *surface, JE_word x, JE_word y, JE_word res, JE_word col, JE_word amt, JE_word xsize, JE_word ysize )
{
	xsize--;
	ysize--;

	for (int z = 1; z <= amt / res; z++)
	{
		JE_barShade(surface, x+2, y+2, x+xsize+2, y+ysize+2);
		fill_rectangle_xy(surface, x, y, x+xsize, y+ysize, col+12);
		fill_rectangle_xy(surface, x, y, x+xsize, y, col+13);
		JE_pix(surface, x, y, col+15);
		fill_rectangle_xy(surface, x, y+ysize, x+xsize, y+ysize, col+11);
		x += xsize + 2;
	}

	amt %= res;
	if (amt > 0)
	{
		JE_barShade(surface, x+2, y+2, x+xsize+2, y+ysize+2);
		fill_rectangle_xy(surface, x,y, x+xsize, y+ysize, col+(12 / res * amt));
	}
}
