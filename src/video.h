/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  video.h
/// \brief TODO

#ifndef VIDEO_H
#define VIDEO_H

#include "opentyr.h"

#include "SDL.h"

#define vga_width 320
#define vga_height 200

extern bool fullscreen_enabled;

extern SDL_Surface *VGAScreen, *VGAScreenSeg;
extern SDL_Surface *game_screen;
extern SDL_Surface *VGAScreen2;

void init_video( void );

int can_init_scaler( unsigned int new_scaler, bool fullscreen );
bool init_scaler( unsigned int new_scaler, bool fullscreen );
bool can_init_any_scaler( bool fullscreen );
bool init_any_scaler( bool fullscreen );

void deinit_video( void );

void JE_clr256( SDL_Surface * );
void JE_showVGA( void );
void scale_and_flip( SDL_Surface * );

#endif /* VIDEO_H */

