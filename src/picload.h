/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  picload.h
/// \brief Loading and displaying internal PCX files (Pics)

#ifndef PICLOAD_H
#define PICLOAD_H

#include "opentyr.h"

extern const JE_byte pic_pal[];

void JE_loadPic(SDL_Surface *screen, JE_byte PCXnumber, JE_boolean storepal );

#endif /* PICLOAD_H */

