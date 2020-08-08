/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  picload.c
/// \brief Loading and displaying internal PCX files (Pics)

#include "file.h"
#include "opentyr.h"
#include "palette.h"
#include "picload.h"
#include "video.h"

#include <string.h>

const JE_byte pic_pal[] = {
	0, 7, 5, 8, 10, 5, 18, 19, 19, 20, 21, 22, 5, /* T2000 */ 23
};

void JE_loadPic(SDL_Surface *screen, JE_byte PCXnumber, JE_boolean storepal )
{
	Uint16 count;
	Uint32 ptr, ptrstop;

	--PCXnumber;

	FILE *f = dir_fopen_die(data_dir(), "tyrian.pic", "rb");

	// read in number of pics
	// T2000: don't assume a certain number of pics
	efread(&count, sizeof(Uint16), 1, f);
	if (PCXnumber >= count)
	{
 		fprintf(stderr, "warning: pic %hhu out of range (0 - %hu)\n", PCXnumber + 1, count - 1);
 		return;
 	}

	fseek(f, PCXnumber * 4, SEEK_CUR);
	efread(&ptr, sizeof(Uint32), 1, f);
	if (PCXnumber == count - 1)
		ptrstop = ftell_eof(f);
	else
		efread(&ptrstop, sizeof(Uint32), 1, f);
	fseek(f, ptr, SEEK_SET);

	unsigned int size = ptrstop - ptr;
	Uint8 *buffer = malloc(size);
	efread(buffer, sizeof(Uint8), size, f);
	fclose(f);

	Uint8 *p = buffer;
	Uint8 *s; // screen pointer, 8-bit specific

	s = (Uint8 *)screen->pixels;

	for (int i = 0; i < 320 * 200; )
	{
		if ((*p & 0xc0) == 0xc0)
		{
			i += (*p & 0x3f);
			memset(s, *(p + 1), (*p & 0x3f));
			s += (*p & 0x3f); p += 2;
		} else {
			i++;
			*s = *p;
			s++; p++;
		}
		if (i && (i % 320 == 0))
		{
			s += screen->pitch - 320;
		}
	}

	free(buffer);

	memcpy(colors, palettes[pic_pal[PCXnumber]], sizeof(colors));

	if (storepal)
		set_palette(colors, 0, 255);
}

