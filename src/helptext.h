/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  helptext.h
/// \brief Functions to read pascal strings, etc

#ifndef HELPTEXT_H
#define HELPTEXT_H

#include "opentyr.h"

#include <stdio.h>

#define HELPTEXT_MISCTEXT_COUNT 68

extern char pName[21][16];
extern char miscText[HELPTEXT_MISCTEXT_COUNT][42];
extern char outputs[9][31];
extern char detailLevel[6][13];
extern char gameSpeedText[6][13];
extern char difficultyNameB[11][21];

void read_encrypted_pascal_string( char *s, int size, FILE *f );
void skip_pascal_string( FILE *f );

void JE_loadHelpText( void );

#endif /* HELPTEXT_H */

