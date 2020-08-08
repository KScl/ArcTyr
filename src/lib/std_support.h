/**** ArcTyr - OpenTyrianArcade ****
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

#ifndef STD_SUPPORT_H
#define STD_SUPPORT_H

/*!
 * \brief Locate a character in a a string.
 * 
 * \param[in] s the string
 * \param[in] c the character
 * \return the pointer to the first occurrence of \p c in \p s if there is an occurrences;
 *         otherwise the pointer to the terminating null character of \p s
 */
char *ot_strchrnul( const char *s, int c );

#endif /* LIB_STD_SUPPORT_H */
