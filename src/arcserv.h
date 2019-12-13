/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcserv.h
/// \brief Arcade service menu functions

#ifndef ARCSERV_H
#define ARCSERV_H

#include "opentyr.h"

#include <setjmp.h>

// Service menu
JE_boolean inServiceMenu;

void ARC_Service( void );

// Entering service menu uses a longjmp, since there's no single "main" loop
jmp_buf service_buffer;

__attribute__((noreturn)) void ARC_EnterService( void );

#endif
