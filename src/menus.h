/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  menus.h
/// \brief Selection menus, etc.

#ifndef MENUS_H
#define MENUS_H

#include "opentyr.h"

extern char episode_name[6][31];

void Menu_newEpisode( void );
void Menu_episodeInterlude( bool silent );

bool Menu_titleScreen( void );
void Menu_hotDebugScreen( void );

#endif /* MENUS_H */

