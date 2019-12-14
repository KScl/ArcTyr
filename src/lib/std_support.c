/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  lib/std_support.h
/// \brief Standard library support functions

#include "std_support.h"

char *ot_strchrnul( const char *s, int c )
{
	for (; *s != c && *s != '\0'; ++s)
		;
	return (char *)s;
}
