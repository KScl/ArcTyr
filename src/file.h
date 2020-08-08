/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  file.h
/// \brief File I/O

#ifndef FILE_H
#define FILE_H

#include "SDL_endian.h"
#include <stdbool.h>
#include <stdio.h>

extern const char *custom_data_dir;

const char *data_dir( void );
const char *arcdata_dir( void );

FILE *dir_fopen( const char *dir, const char *file, const char *mode );
FILE *dir_fopen_warn( const char *dir, const char *file, const char *mode );
FILE *dir_fopen_die( const char *dir, const char *file, const char *mode );

bool dir_file_exists( const char *dir, const char *file );

long ftell_eof( FILE *f );

// endian-swapping fread/fwrite that die if the expected amount cannot be read/written
size_t efread( void *buffer, size_t size, size_t num, FILE *stream );
size_t efwrite( const void *buffer, size_t size, size_t num, FILE *stream );

#endif // FILE_H

