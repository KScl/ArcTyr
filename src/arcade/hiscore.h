/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcade/hiscore.h
/// \brief High score tables and screens, saving, etc

#ifndef HISCORE_H
#define HISCORE_H

#include "../opentyr.h"
#include "../player.h"

#define NUM_HIGH_SCORE_ENTRIES 20

typedef struct {
	char name[10];
	uint cash, ship;
	JE_boolean isNew;
} HighScoreEntry;

JE_boolean HighScore_InsertName( Player *pl );
void HighScore_UpdateName( Player *pl );

JE_boolean HighScore_Leading( Player *pl );

JE_boolean HighScore_ShouldShow( void );
void HighScore_ClearNewFlag( void );

#endif
