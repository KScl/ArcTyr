/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef MAININT_H
#define MAININT_H

#include "config.h"
#include "opentyr.h"
#include "palette.h"
#include "player.h"
#include "sprite.h"

extern JE_word textErase;
extern JE_boolean useLastBank;

extern int cameraXFocus;

void JE_drawTextWindow( const char *text );
void JE_initPlayerData( void );
void JE_gammaCorrect_func( JE_byte *col, JE_real r );
void JE_gammaCorrect( Palette *colorBuffer, JE_byte gamma );

bool load_next_demo( void );

void JE_drawPortConfigButtons( void );
void JE_outCharGlow( JE_word x, JE_word y, const char *s );

void JE_playCredits( void );
void JE_endLevelAni( void );
void JE_inGameDisplays( void );

void JE_playerMovement( Player *this_player, JE_byte playerNum, JE_word shipGr_ );
void JE_mainGamePlayerFunctions( void );

void JE_playerCollide( Player *this_player, JE_byte playerNum );

void JE_drawTextWindowColorful( const char *text );
void JE_colorFromPWeapon( uint pw, JE_byte *c, JE_byte *bri );
char *JE_textColorFromPWeapon( uint pw );
char *JE_trim( const char *str );

#endif /* MAININT_H */

