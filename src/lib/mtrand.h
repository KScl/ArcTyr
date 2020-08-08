/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  TODO
/// \brief

#ifndef LIB_MTRAND_H
#define LIB_MTRAND_H

#define MT_RAND_MAX 0xffffffffUL

void mt_srand( unsigned long s );
unsigned long mt_rand( void );
float mt_rand_1( void );
float mt_rand_lt1( void );

#endif /* LIB_MTRAND_H */

