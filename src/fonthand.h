/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  fonthand.h
/// \brief Older text drawing routines

#ifndef FONTHAND_H
#define FONTHAND_H

#include "opentyr.h"

#include "SDL.h"

#define PART_SHADE 0
#define FULL_SHADE 1
#define DARKEN     2
#define TRICK      3
#define NO_SHADE 255

extern const int font_ascii[256];

extern JE_byte textGlowFont;
extern JE_boolean levelWarningDisplay;
extern JE_byte levelWarningLines;
extern char levelWarningText[12][61];
extern JE_boolean warningRed;
extern JE_byte warningSoundDelay;
extern JE_word armorShipDelay;
extern JE_byte warningCol;
extern JE_shortint warningColChange;

void JE_dString( SDL_Surface * screen, int x, int y, const char *s, unsigned int font );

int JE_fontCenter( const char *s, unsigned int font );
int JE_textWidth( const char *s, unsigned int font );
void JE_textShade( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, int brightness, unsigned int shadetype );
void JE_outText( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, int brightness );
void JE_outTextModify( SDL_Surface * screen, int x, int y, const char *s, unsigned int filter, unsigned int brightness, unsigned int font );
void JE_outTextAdjust( SDL_Surface * screen, int x, int y, const char *s, unsigned int filter, int brightness, unsigned int font, bool shadow );
void JE_outTextAndDarken( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, unsigned int brightness, unsigned int font );

void JE_updateWarning( SDL_Surface * screen );

void JE_saveTextGlow( int x, int y, const char *s );
void JE_drawTextGlow( SDL_Surface * screen );
void JE_outTextGlow( SDL_Surface * screen, int x, int y, const char *s );

#endif /* FONTHAND_H */

