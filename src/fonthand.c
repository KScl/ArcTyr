/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  fonthand.c
/// \brief Older text drawing routines

#include "fonthand.h"
#include "input.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"
#include "sprite.h"
#include "vga256d.h"
#include "video.h"

const int font_ascii[256] =
{
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  26,  33,  60,  61,  62,  -1,  32,  64,  65,  63,  84,  29,  83,  28,  80, //  !"#$%&'()*+,-./
	 79,  70,  71,  72,  73,  74,  75,  76,  77,  78,  31,  30,  -1,  85,  -1,  27, // 0123456789:;<=>?
	 -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14, // @ABCDEFGHIJKLMNO
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  68,  82,  69,  -1,  -1, // PQRSTUVWXYZ[\]^_
	 -1,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48, // `abcdefghijklmno
	 49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  66,  81,  67,  -1,  -1, // pqrstuvwxyz{|}~⌂

	 86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, // ÇüéâäàåçêëèïîìÄÅ
	102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, // ÉæÆôöòûùÿÖÜ¢£¥₧ƒ
	118, 119, 120, 121, 122, 123, 124, 125, 126,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // áíóúñÑªº¿
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
};

/* shape constants included in newshape.h */

JE_byte textGlowFont;

JE_boolean levelWarningDisplay;
JE_byte levelWarningLines;
// T2000: The ending text requires 12 lines
char levelWarningText[12][61]; /* [1..12] of string [60] */
JE_boolean warningRed;

JE_byte warningSoundDelay;
JE_word armorShipDelay;
JE_byte warningCol;
JE_shortint warningColChange;

void JE_dString( SDL_Surface * screen, int x, int y, const char *s, unsigned int font )
{
	const int defaultBrightness = -3;

	int bright = 0;

	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		switch (s[i])
		{
		case ' ':
			x += 6;
			break;

		case '~':
			bright = (bright == 0) ? 2 : 0;
			break;

		default:
			if (sprite_id != -1)
			{
				blit_sprite_dark(screen, x + 2, y + 2, font, sprite_id, false);
				blit_sprite_hv_unsafe(screen, x, y, font, sprite_id, 0xf, defaultBrightness + bright);

				x += sprite(font, sprite_id)->width + 1;
			}
			break;
		}
	}
}

int JE_fontCenter( const char *s, unsigned int font )
{
	return 160 - (JE_textWidth(s, font) / 2);
}

int JE_textWidth( const char *s, unsigned int font )
{
	int x = 0;

	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		if (s[i] == ' ')
			x += 6;
		else if (sprite_id != -1)
			x += sprite(font, sprite_id)->width + 1;
	}

	return x;
}

void JE_textShade( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, int brightness, unsigned int shadetype )
{
	switch (shadetype)
	{
		case PART_SHADE:
			JE_outText(screen, x+1, y+1, s, 0, -1);
			JE_outText(screen, x, y, s, colorbank, brightness);
			break;
		case FULL_SHADE:
			JE_outText(screen, x-1, y, s, 0, -1);
			JE_outText(screen, x+1, y, s, 0, -1);
			JE_outText(screen, x, y-1, s, 0, -1);
			JE_outText(screen, x, y+1, s, 0, -1);
			JE_outText(screen, x, y, s, colorbank, brightness);
			break;
		case DARKEN:
			JE_outTextAndDarken(screen, x+1, y+1, s, colorbank, brightness, TINY_FONT);
			break;
		case TRICK:
			JE_outTextModify(screen, x, y, s, colorbank, brightness, TINY_FONT);
			break;
	}
}

void JE_outText( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, int brightness )
{
	int bright = 0;

	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		switch (s[i])
		{
		case ' ':
			x += 6;
			break;

		case '~':
			bright = (bright == 0) ? 4 : 0;
			break;

		default:
			if (sprite_id != -1 && sprite_exists(TINY_FONT, sprite_id))
			{
				if (brightness >= 0)
					blit_sprite_hv_unsafe(screen, x, y, TINY_FONT, sprite_id, colorbank, brightness + bright);
				else
					blit_sprite_dark(screen, x, y, TINY_FONT, sprite_id, true);

				x += sprite(TINY_FONT, sprite_id)->width + 1;
			}
			break;
		}
	}
}

void JE_outTextModify( SDL_Surface * screen, int x, int y, const char *s, unsigned int filter, unsigned int brightness, unsigned int font )
{
	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		if (s[i] == ' ')
		{
			x += 6;
		}
		else if (sprite_id != -1)
		{
			blit_sprite_hv_blend(screen, x, y, font, sprite_id, filter, brightness);

			x += sprite(font, sprite_id)->width + 1;
		}
	}
}

void JE_outTextAdjust( SDL_Surface * screen, int x, int y, const char *s, unsigned int filter, int brightness, unsigned int font, JE_boolean shadow )
{
	int bright = 0;

	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		switch (s[i])
		{
		case ' ':
			x += 6;
			break;

		case '~':
			bright = (bright == 0) ? 4 : 0;
			break;

		default:
			if (sprite_id != -1 && sprite_exists(TINY_FONT, sprite_id))
			{
				if (shadow)
					blit_sprite_dark(screen, x + 2, y + 2, font, sprite_id, false);
				blit_sprite_hv(screen, x, y, font, sprite_id, filter, brightness + bright);

				x += sprite(font, sprite_id)->width + 1;
			}
			break;
		}
	}
}

void JE_outTextAndDarken( SDL_Surface * screen, int x, int y, const char *s, unsigned int colorbank, unsigned int brightness, unsigned int font )
{
	int bright = 0;

	for (int i = 0; s[i] != '\0'; ++i)
	{
		int sprite_id = font_ascii[(unsigned char)s[i]];

		switch (s[i])
		{
		case ' ':
			x += 6;
			break;

		case '~':
			bright = (bright == 0) ? 4 : 0;
			break;

		default:
			if (sprite_id != -1 && sprite_exists(TINY_FONT, sprite_id))
			{
				blit_sprite_dark(screen, x + 1, y + 1, font, sprite_id, false);
				blit_sprite_hv_unsafe(screen, x, y, font, sprite_id, colorbank, brightness + bright);

				x += sprite(font, sprite_id)->width + 1;
			}
			break;
		}
	}
}

void JE_updateWarning( SDL_Surface * screen )
{
	if (delaycount2() == 0)
	{ /*Update Color Bars*/

		warningCol += warningColChange;
		if (warningCol > 14 * 16 + 10 || warningCol < 14 * 16 + 4)
		{
			warningColChange = -warningColChange;
		}
		fill_rectangle_xy(screen, 0, 0, 319, 5, warningCol);
		fill_rectangle_xy(screen, 0, 194, 319, 199, warningCol);
		JE_showVGA();

		setjasondelay2(6);

		if (warningSoundDelay > 0)
		{
			warningSoundDelay--;
		}
		else
		{
			warningSoundDelay = 14;
			JE_playSampleNum(S_WARNING);
		}
	}
}

// ---
// End Level Animation Glowing Text
// ---

static struct {
	char buf[48];
	int x, y;
} tg_data[3];
static uint num_tg_data = 0;

void JE_saveTextGlow( int x, int y, const char *s )
{
	if (num_tg_data >= 3)
		return;

	strncpy(tg_data[num_tg_data].buf, s, 48);
	tg_data[num_tg_data].x = x;
	tg_data[num_tg_data].y = y;
	++num_tg_data;
}

void JE_drawTextGlow( SDL_Surface * screen )
{
	uint z = 0, n;
	const JE_byte c = 15;
	const JE_byte brightness[] =
	{
		-9, -9, -8, -8, -7, -7, -6, -6, -5, -5,
		-4, -4, -3, -3, -2, -2, -1, -1,  0,  0,
		 1,  1,  2,  2,  2,  2,  1,  1,  0,  0,
		-1, -1, -2, -2, -3, -3, -4, -4
	};

	for (n = 0; n < num_tg_data; ++n)
	{
		JE_outTextAdjust(screen, tg_data[n].x - 1, tg_data[n].y,     tg_data[n].buf, 0, -12, textGlowFont, false);
		JE_outTextAdjust(screen, tg_data[n].x,     tg_data[n].y - 1, tg_data[n].buf, 0, -12, textGlowFont, false);
		JE_outTextAdjust(screen, tg_data[n].x + 1, tg_data[n].y,     tg_data[n].buf, 0, -12, textGlowFont, false);
		JE_outTextAdjust(screen, tg_data[n].x,     tg_data[n].y + 1, tg_data[n].buf, 0, -12, textGlowFont, false);		
	}

	if (hasRequestedToSkip)
		z = COUNTOF(brightness) - 1;

	for (; z < COUNTOF(brightness); ++z)
	{
		setjasondelay(frameCountMax);

		for (n = 0; n < num_tg_data; ++n)
			JE_outTextAdjust(screen, tg_data[n].x, tg_data[n].y, tg_data[n].buf, c, brightness[z], textGlowFont, false);

		if (!hasRequestedToSkip && I_checkSkipAndStatus())
		{
			z = COUNTOF(brightness) - 2;
			hasRequestedToSkip = true;
		}

		JE_showVGA();
		wait_delay();
	}
	num_tg_data = 0;
}

void JE_outTextGlow( SDL_Surface * screen, int x, int y, const char *s )
{
	JE_saveTextGlow(x, y, s);
	JE_drawTextGlow(screen);
}
