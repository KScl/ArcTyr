/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  mod/patcher.h
/// \brief Level and episode patchers

#ifndef MOD_PATCHER_H
#define MOD_PATCHER_H

#include "../varz.h"

void MOD_PatcherInit( JE_byte episode, JE_byte level );
bool MOD_Patcher( struct JE_EventRecType *allEvs, JE_word *ev );
void MOD_PatcherClose( void );

#endif
