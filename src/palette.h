/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  palette.h
/// \brief Palette fade effects, etc.

#ifndef PALETTE_H
#define PALETTE_H

#include "opentyr.h"

#include "SDL.h"

typedef SDL_Color Palette[256];

extern Palette *palettes;
extern int palette_count;

extern Uint32 rgb_palette[256], yuv_palette[256];

extern Palette colors; // TODO: get rid of this

void JE_loadPals( void );
void free_pals( void );

void set_palette( Palette colors, unsigned int first_color, unsigned int last_color );
void set_colors( SDL_Color color, unsigned int first_color, unsigned int last_color );

void init_step_fade_palette( int diff[256][3], Palette colors, unsigned int first_color, unsigned int last_color );
void init_step_fade_solid( int diff[256][3], SDL_Color color, unsigned int first_color, unsigned int last_color );
void step_fade_palette( int diff[256][3], int steps, unsigned int first_color, unsigned int last_color );

void fade_palette( Palette colors, int steps, unsigned int first_color, unsigned int last_color );
void fade_solid( SDL_Color color, int steps, unsigned int first_color, unsigned int last_color );

void fade_black( int steps );
void fade_white( int steps );

#endif /* PALETTE_H */

