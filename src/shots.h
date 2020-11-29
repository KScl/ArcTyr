/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  shots.h
/// \brief Player shot management

#ifndef SHOTS_H
#define SHOTS_H
#include "opentyr.h"

typedef struct {
	JE_integer shotX, shotY, shotXM, shotYM;
	JE_integer shotXC, shotYC, shotXCW, shotYCW, shotXCWmax, shotYCWmax;
	JE_boolean shotComplicated;
	JE_integer shotDevX, shotDirX, shotDevY, shotDirY, shotCirSizeX, shotCirSizeY;
	JE_byte shotTrail;
	JE_word shotGr, shotAni, shotAniMax;
	JE_byte shotBlastFilter, playerNumber, aimAtEnemy, aimDelay, aimDelayMax;

	// New data
	JE_word shotDmg, shrapnel;
	JE_byte ice;
	JE_boolean infinite;
} PlayerShotDataType;

#define MAX_PWEAPON     128 /* formerly 81 */
extern PlayerShotDataType playerShotData[MAX_PWEAPON];
extern JE_byte shotAvail[MAX_PWEAPON];

/** Used in the shop to show weapon previews. */
void simulate_player_shots( void );

/** Moves and draws a shot. Does \b not collide it with enemies.
 * \return False if the shot went offscreen, true otherwise.
 */
bool player_shot_move_and_draw(
		int shot_id, bool* out_is_special,
		int* out_shotx, int* out_shoty,
		JE_word* out_special_radiusw, JE_word* out_special_radiush );

/** Creates a player shot. */
JE_integer player_shot_create( JE_word portnum, uint shot_i, JE_word px, JE_word py, JE_word wpnum, JE_byte playernum );

#endif // SHOTS_H
