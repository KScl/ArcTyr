/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  nortvars.h
/// \brief TODO These functions are unsorted

#ifndef NORTVARS_H
#define NORTVARS_H

#include "opentyr.h"

void JE_dBar3( SDL_Surface *surface, JE_integer x,  JE_integer y,  JE_integer num,  JE_integer col );
void JE_barDrawShadow( SDL_Surface *surface, JE_word x, JE_word y, JE_word res, JE_word col, JE_word amt, JE_word xsize, JE_word ysize );

#endif /* NORTVARS_H */

