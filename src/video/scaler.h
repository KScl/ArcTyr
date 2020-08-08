/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  video/scaler.h
/// \brief Generic scaling handler

#ifndef VIDEO_SCALER_H
#define VIDEO_SCALER_H

// You may enable or disable additional scalers by uncommenting these.
//#define ENABLE_SCALENX_SCALER
//#define ENABLE_HQNX_SCALER

#include "../opentyr.h"

#include "SDL.h"

typedef void (*ScalerFunction)( SDL_Surface *dst, SDL_Surface *src );

struct Scalers
{
	int width, height;
	ScalerFunction scaler8, scaler16, scaler32;
	const char *name;
};

extern uint scaler;
extern const struct Scalers scalers[];
extern const uint scalers_count;

void set_scaler_by_name( const char *name );

#ifdef ENABLE_SCALENX_SCALER
void scale2x_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
void scale2x_16( SDL_Surface *src_surface, SDL_Surface *dst_surface );
void scale3x_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
void scale3x_16( SDL_Surface *src_surface, SDL_Surface *dst_surface );
#endif

#ifdef ENABLE_HQNX_SCALER
void hq2x_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
void hq3x_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
void hq4x_32( SDL_Surface *src_surface, SDL_Surface *dst_surface );
#endif

#endif /* VIDEO_SCALER_H */

