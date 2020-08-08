/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  tyrian2.h
/// \brief TODO

#ifndef TYRIAN2_H
#define TYRIAN2_H

#include "opentyr.h"

#include "varz.h"
#include "helptext.h"

//#define EVENT_LOGGING

void intro_logos( void );

#define EVENT_MAXIMUM 2500

typedef struct
{
	Uint8 link_num;
	Uint8 armor;
	Uint8 color;
}
boss_bar_t;

extern boss_bar_t boss_bar[2];

extern char tempStr[31];

void JE_createNewEventEnemy( JE_byte enemytypeofs, JE_word enemyoffset, JE_byte shapeTableOverride );

uint JE_makeEnemy( struct JE_SingleEnemyType *enemy, Uint16 eDatI, JE_byte shapeTableOverride );

void JE_eventJump( JE_word jump );

void JE_barX ( JE_word x1, JE_word y1, JE_word x2, JE_word y2, JE_byte col );

Sint16 JE_newEnemy( int enemyOffset, Uint16 eDatI, JE_byte shapeTableOverride );
void JE_drawEnemy( int enemyOffset );
void JE_starShowVGA( void );

void JE_main( void );
void JE_loadMap( void );
void JE_displayText( void );

bool JE_searchFor( JE_byte PLType, JE_byte* out_index );
void JE_eventSystem( void );

void draw_boss_bar( void );

#endif /* TYRIAN2_H */

