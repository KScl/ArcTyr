/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  params.h
/// \brief Parameters on load.

#ifndef PARAMS_H
#define PARAMS_H

#include "opentyr.h"

extern JE_byte shutdownCode;
extern JE_boolean goToWeaponCreator;
extern JE_boolean isFirstRun;

void JE_paramCheck( int argc, char *argv[] );

#endif /* PARAMS_H */

