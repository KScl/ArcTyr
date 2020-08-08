/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  pcxload.h
/// \brief Loading and displaying PCX files

#include "file.h"
#include "opentyr.h"
#include "palette.h"
#include "pcxload.h"
#include "video.h"

void JE_loadPCX( const char *dir, const char *file ) // this is only meant to load tshp2.pcx
{
	Uint8 *s = VGAScreen->pixels; /* 8-bit specific */
	
	FILE *f = dir_fopen_die(dir, file, "rb");
	
	fseek(f, -769, SEEK_END);
	
	if (fgetc(f) == 12)
	{
		for (int i = 0; i < 256; i++)
		{
			efread(&colors[i].r, 1, 1, f);
			efread(&colors[i].g, 1, 1, f);
			efread(&colors[i].b, 1, 1, f);
		}
	}
	
	fseek(f, 128, SEEK_SET);
	
	for (int i = 0; i < 320 * 200; )
	{
		Uint8 p = fgetc(f);
		if ((p & 0xc0) == 0xc0)
		{
			i += (p & 0x3f);
			memset(s, fgetc(f), (p & 0x3f));
			s += (p & 0x3f);
		} else {
			i++;
			*s = p;
			s++;
		}
		if (i && (i % 320 == 0))
		{
			s += VGAScreen->pitch - 320;
		}
	}
	
	fclose(f);
}
