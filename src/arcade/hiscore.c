/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcade/hiscore.c
/// \brief High score tables and screens, saving, etc

#include "hiscore.h"

#include "../opentyr.h"
#include "../player.h"

static JE_boolean displayScreenNext = false;
static HighScoreEntry highScores[NUM_HIGH_SCORE_ENTRIES] = {
	{"Trent    ", 100000, 4, false},
	{"Dougan   ",  90000, 7, false},
	{"Transon  ",  80000, 6, false},
	{"Vykromod ",  70000, 3, false},
	{"Beppo    ",  60000, 1, false},
	{"Steffan  ",  50000, 4, false},
	{"Lori     ",  45000, 2, false},
	{"Angel    ",  40000, 9, false},
	{"Jazz     ",  35000, 2, false},
	{"Crystal  ",  30000, 5, false},
	{"Javi     ",  25000, 7, false},
	{"Cossette ",  20000, 2, false},
	{"Jill     ",  15000, 1, false},
	{"Raven    ",  10000, 1, false},
	{"Spaz     ",   7000, 2, false},
	{"849      ",   5000, 1, false},
	{"Darcy    ",   4000, 1, false},
	{"Jean-Paul",   3000, 1, false},
	{"Jake     ",   2000, 1, false},
	{"Devan    ",   1000, 2, false},
};

JE_boolean HighScore_InsertName( Player *pl )
{
	for (int i = 0; i < NUM_HIGH_SCORE_ENTRIES; ++i)
	{
		if (pl->cash >= highScores[i].cash)
		{
			// Shift down high scores if necessary
			if (i < NUM_HIGH_SCORE_ENTRIES - 1)
				memmove(&highScores[i+1], &highScores[i], sizeof(HighScoreEntry) * ((NUM_HIGH_SCORE_ENTRIES - 1) - i));
			snprintf(highScores[i].name, sizeof(highScores[i].name), "Unknown");
			highScores[i].cash = pl->cash;
			highScores[i].isNew = true;
			displayScreenNext = true;

			pl->arc.hsPos = i;
			{
				Player *otherPl = PL_OtherPlayer(pl);
				if (otherPl->arc.hsPos >= i)
					++otherPl->arc.hsPos;
			}
			return true;
		}
	}
	return false;
}

void HighScore_UpdateName( Player *pl )
{
	if (pl->arc.hsPos < 0 && pl->arc.hsPos >= NUM_HIGH_SCORE_ENTRIES)
		memcpy(highScores[pl->arc.hsPos].name, pl->arc.hsName, sizeof(pl->arc.hsName));
}


//
// Ingame Functions
//

JE_boolean HighScore_Leading( Player *pl )
{
	Player *otherPl = PL_OtherPlayer(pl);
	if (otherPl->player_status > STATUS_NONE && otherPl->cash > pl->cash)
		return false;
	return (pl->cash >= highScores[0].cash);
}


//
// High Score Screen
//

void HighScore_Screen( void )
{
	
}

// Used to determine whether to show high score screen after credits/game over
JE_boolean HighScore_ShouldShow( void )
{
	return displayScreenNext;
}

void HighScore_ClearNewFlag( void )
{
	for (int i = 0; i < NUM_HIGH_SCORE_ENTRIES; ++i)
		highScores[i].isNew = false;
	displayScreenNext = false;
}
