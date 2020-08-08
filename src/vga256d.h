/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  vga256d.h
/// \brief TODO

#ifndef VGA256D_H
#define VGA256D_H

#include "opentyr.h"

void JE_pix( SDL_Surface *surface, int x, int y, JE_byte c );
void JE_pix3( SDL_Surface *surface, int x, int y, JE_byte c );
void JE_rectangle( SDL_Surface *surface, int a, int b, int c, int d, int e );

void fill_rectangle_xy( SDL_Surface *, int x, int y, int x2, int y2, Uint8 color );

void JE_barShade( SDL_Surface *surface, int a, int b, int c, int d );
void JE_barBright( SDL_Surface *surface, int a, int b, int c, int d );

static inline void fill_rectangle_hw( SDL_Surface *surface, int x, int y, uint h, uint w, Uint8 color )
{
	SDL_Rect rect = { x, y, h, w };
	SDL_FillRect(surface, &rect, color);
}

void draw_segmented_gauge( SDL_Surface *surface, int x, int y, Uint8 color, uint segment_width, uint segment_height, uint segment_value, uint value );

#endif /* VGA256D_H */

