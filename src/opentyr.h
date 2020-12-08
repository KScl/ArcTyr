/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  opentyr.h
/// \brief Global defines

#ifndef OPENTYR_H
#define OPENTYR_H

#define ENABLE_DEVTOOLS
#undef WITH_NETWORK

// snprintf truncation is intended; ignore warnings about it occurring
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

#include "SDL_types.h"
#include <math.h>
#include <stdbool.h>

typedef struct {
	char s[21];
	char l[256];
} TempStringBuffer;
TempStringBuffer tmpBuf;

#ifndef COUNTOF
#define COUNTOF(x) ((unsigned)(sizeof(x) / sizeof *(x)))  // use only on arrays!
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifndef M_PI
#define M_PI    3.14159265358979323846  // pi
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923  // pi/2
#endif
#ifndef M_PI_4
#define M_PI_4  0.78539816339744830962  // pi/4
#endif

typedef unsigned int uint;
typedef unsigned long ulong;

// Pascal types, yuck.
typedef Sint32 JE_longint;
typedef Sint16 JE_integer;
typedef Sint8  JE_shortint;
typedef Uint16 JE_word;
typedef Uint8  JE_byte;
typedef bool   JE_boolean;
typedef char   JE_char;
typedef float  JE_real;

#define TYRIAN_VERSION "2.1"

extern const char *opentyrian_str, *opentyrian_version, *opentyrian_date, *opentyrian_time;

extern JE_byte isInGame;

// T2000
// Used to adjust graphics and such for Tyrian 2000 data
// Don't use when loading episodes, allow mixing of those
extern bool tyrian2000detected;

#define STUB() do { printf("stubbed: %s\n", __func__); } while (false)

__attribute__((noreturn)) void JE_tyrianHalt( JE_byte code ); /* This ends the game */

#endif /* OPENTYR_H */

