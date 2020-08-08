/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  mainint.h
/// \brief TODO

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

