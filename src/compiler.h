/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
 /// \file  compiler.h
 /// \brief Compiler specific defines

#ifndef COMPILER_H
#define COMPILER_H

#if defined(_MSC_VER)
// --
// MSVC specific defines
// --

#define FUNCNORETURN __declspec(noreturn)

// ---
// End MSVC section
// ---
#elif defined(__GNUC__) || defined
// ---
// GCC specific defines
// ---

#define FUNCNORETURN __attribute__((noreturn))

// snprintf truncation is intended; ignore warnings about it occurring
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

// ---
// End GCC section
// ---
#else
// ---
// Generic empty defines for other compilers
// ---

#define FUNCNORETURN

// ---
// End generic section
// ---
#endif

#endif