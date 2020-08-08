/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  mod/patcher.h
/// \brief Level and episode patchers

#ifndef MOD_PATCHER_H
#define MOD_PATCHER_H

enum {
	PATCH_DISABLED = 0,
	PATCH_EPISODE_1,
	PATCH_EPISODE_2,
	PATCH_EPISODE_3,
	PATCH_EPISODE_4,
	PATCH_EPISODE_1_T2K,
	PATCH_EPISODE_2_T2K,
	PATCH_EPISODE_3_T2K,
	PATCH_EPISODE_4_T2K,
	PATCH_EPISODE_5_T2K,
};

#include "../varz.h"

void MOD_PatcherSetup( const char *levelFile );
void MOD_PatcherInit( JE_byte level );
bool MOD_Patcher( struct JE_EventRecType *allEvs, JE_word *ev );
void MOD_PatcherClose( void );

#endif
