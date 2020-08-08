/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  font.h
/// \brief Revamped text drawing routines

#ifndef FONT_H
#define FONT_H

#include "SDL.h"
#include <stdbool.h>

typedef enum
{
	large_font = 0,
	normal_font = 1,
	small_font = 2
}
Font;

typedef enum
{
	left_aligned,
	centered,
	right_aligned
}
FontAlignment;

void draw_font_hv_shadow( SDL_Surface *, int x, int y, const char *text, Font, FontAlignment, Uint8 hue, Sint8 value, bool black, int shadow_dist );
void draw_font_hv_full_shadow( SDL_Surface *, int x, int y, const char *text, Font, FontAlignment, Uint8 hue, Sint8 value, bool black, int shadow_dist );

void draw_font_hv( SDL_Surface *, int x, int y, const char *text, Font, FontAlignment, Uint8 hue, Sint8 value );
void draw_font_hv_blend( SDL_Surface *, int x, int y, const char *text, Font, FontAlignment, Uint8 hue, Sint8 value );
void draw_font_dark( SDL_Surface *, int x, int y, const char *text, Font, FontAlignment, bool black );

#endif // FONT_H
