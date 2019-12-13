/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  devextra.c
/// \brief Extra menus and functions to aide in development

#ifndef DEVEXTRA_H
#define DEVEXTRA_H

#include "opentyr.h"

void DEV_WeaponCreator( uint start_weap );

void DEV_RecordDemoInit( void );
void DEV_RecordDemoStart( void );
void DEV_RecordDemoInput( void );
void DEV_RecordDemoEnd( void );

#endif
