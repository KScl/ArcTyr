/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
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

