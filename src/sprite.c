/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  sprite.c
/// \brief Loading and displaying game sprites

#include "file.h"
#include "opentyr.h"
#include "sprite.h"
#include "video.h"

#include <assert.h>
#include <ctype.h>

Sprite_array sprite_table[SPRITE_TABLES_MAX];

Sprite2_array shotShapes[2];
Sprite2_array iconShapes;
Sprite2_array pickupShapes;
Sprite2_array shipShapes, shipShapesT2K;
Sprite2_array eShapes[4];
Sprite2_array shapes6;

void load_sprites_file( unsigned int table, const char *dir, const char *filename )
{
	free_sprites(table);
	
	FILE *f = dir_fopen_die(dir, filename, "rb");
	
	load_sprites(table, f);
	
	fclose(f);
}

void load_sprites( unsigned int table, FILE *f )
{
	free_sprites(table);
	
	Uint16 temp;
	efread(&temp, sizeof(Uint16), 1, f);
	
	sprite_table[table].count = temp;
	
	assert(sprite_table[table].count <= SPRITES_PER_TABLE_MAX);
	
	for (unsigned int i = 0; i < sprite_table[table].count; ++i)
	{
		Sprite * const cur_sprite = sprite(table, i);
		
		if (!getc(f)) // sprite is empty
			continue;
		
		efread(&cur_sprite->width,  sizeof(Uint16), 1, f);
		efread(&cur_sprite->height, sizeof(Uint16), 1, f);
		efread(&cur_sprite->size,   sizeof(Uint16), 1, f);
		
		cur_sprite->data = malloc(cur_sprite->size);
		
		efread(cur_sprite->data, sizeof(Uint8), cur_sprite->size, f);
	}
}

void free_sprites( unsigned int table )
{
	for (unsigned int i = 0; i < sprite_table[table].count; ++i)
	{
		Sprite * const cur_sprite = sprite(table, i);
		
		cur_sprite->width  = 0;
		cur_sprite->height = 0;
		cur_sprite->size   = 0;
		
		free(cur_sprite->data);
		cur_sprite->data = NULL;
	}
	
	sprite_table[table].count = 0;
}

// does not clip on left or right edges of surface
void blit_sprite( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = *data;
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_blend( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = (*data & 0xf0) | (((*pixels & 0x0f) + (*data & 0x0f)) / 2);
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
// unsafe because it doesn't check that value won't overflow into hue
// we can replace it when we know that we don't rely on that 'feature'
void blit_sprite_hv_unsafe( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = hue | ((*data & 0x0f) + value);
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_hv( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
			{
				Uint8 temp_value = (*data & 0x0f) + value;
				if (temp_value > 0xf)
					temp_value = (temp_value >= 0x1f) ? 0x0 : 0xf;
				
				*pixels = hue | temp_value;
			}
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_hv_blend( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	hue <<= 4;
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
			{
				Uint8 temp_value = (*data & 0x0f) + value;
				if (temp_value > 0xf)
					temp_value = (temp_value >= 0x1f) ? 0x0 : 0xf;
				
				*pixels = hue | (((*pixels & 0x0f) + temp_value) / 2);
			}
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite_dark( SDL_Surface *surface, int x, int y, unsigned int table, unsigned int index, bool black )
{
	if (index >= sprite_table[table].count || !sprite_exists(table, index))
	{
		assert(false);
		return;
	}
	
	const Sprite * const cur_sprite = sprite(table, index);
	
	const Uint8 *data = cur_sprite->data;
	const Uint8 * const data_ul = data + cur_sprite->size;
	
	const unsigned int width = cur_sprite->width;
	unsigned int x_offset = 0;
	
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	for (; data < data_ul; ++data)
	{
		switch (*data)
		{
		case 255:  // transparent pixels
			data++;  // next byte tells how many
			pixels += *data;
			x_offset += *data;
			break;
			
		case 254:  // next pixel row
			pixels += width - x_offset;
			x_offset = width;
			break;
			
		case 253:  // 1 transparent pixel
			pixels++;
			x_offset++;
			break;
			
		default:  // set a pixel
			if (pixels >= pixels_ul)
				return;
			if (pixels >= pixels_ll)
				*pixels = black ? 0x00 : ((*pixels & 0xf0) | ((*pixels & 0x0f) / 2));
			
			pixels++;
			x_offset++;
			break;
		}
		if (x_offset >= width)
		{
			pixels += surface->pitch - x_offset;
			x_offset = 0;
		}
	}
}

void loadCompShapesArc( Sprite2_array *sprite2s, const char *filename )
{
	//snprintf(tmpBuf.s, sizeof(tmpBuf.s), "arc%02u.shp", id);

	FILE *f = dir_fopen_die(arcdata_dir(), filename, "rb");

	sprite2s->size = ftell_eof(f);
	
	JE_loadCompShapesB(sprite2s, f);
	
	fclose(f);
}

void JE_loadCompShapes( Sprite2_array *sprite2s, char s )
{
	char buffer[20];
	snprintf(buffer, sizeof(buffer), "newsh%c.shp", tolower((unsigned char)s));
	
	FILE *f = dir_fopen_die(data_dir(), buffer, "rb");
	
	sprite2s->size = ftell_eof(f);
	
	JE_loadCompShapesB(sprite2s, f);
	
	fclose(f);
}

void JE_loadCompShapesB( Sprite2_array *sprite2s, FILE *f )
{
	free_sprite2s(sprite2s);
	
	sprite2s->data = malloc(sizeof(Uint8) * sprite2s->size);
	efread(sprite2s->data, sizeof(Uint8), sprite2s->size, f);
}

void free_sprite2s( Sprite2_array *sprite2s )
{
	free(sprite2s->data);
	sprite2s->data = NULL;
}

// does not clip on left or right edges of surface
void blit_sprite2( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit

	// Note: hopefully only necessary when debugging t2k files
/*	if (index == 0)
	{
		printf("warning: tried to blit spr2 index 0\n");
		return;
	}
*/

	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = *data;
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_blend( SDL_Surface *surface,  int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = (((*data & 0x0f) + (*pixels & 0x0f)) / 2) | (*data & 0xf0);
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_darken( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = ((*pixels & 0x0f) / 2) + (*pixels & 0xf0);
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2_filter( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index, Uint8 filter )
{
	assert(surface->format->BitsPerPixel == 8);
	Uint8 *             pixels =    (Uint8 *)surface->pixels + (y * surface->pitch) + x;
	const Uint8 * const pixels_ll = (Uint8 *)surface->pixels,  // lower limit
	            * const pixels_ul = (Uint8 *)surface->pixels + (surface->h * surface->pitch);  // upper limit
	
	const Uint8 *data = sprite2s.data + SDL_SwapLE16(((Uint16 *)sprite2s.data)[index - 1]);
	
	for (; *data != 0x0f; ++data)
	{
		pixels += *data & 0x0f;                   // second nibble: transparent pixel count
		unsigned int count = (*data & 0xf0) >> 4; // first nibble: opaque pixel count
		
		if (count == 0) // move to next pixel row
		{
			pixels += VGAScreen->pitch - 12;
		}
		else
		{
			while (count--)
			{
				++data;
				
				if (pixels >= pixels_ul)
					return;
				if (pixels >= pixels_ll)
					*pixels = filter | (*data & 0x0f);
				
				++pixels;
			}
		}
	}
}

// does not clip on left or right edges of surface
void blit_sprite2x2( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	blit_sprite2(surface, x,      y,      sprite2s, index);
	blit_sprite2(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2(surface, x + 12, y + 14, sprite2s, index + 20);
}

// does not clip on left or right edges of surface
void blit_sprite2x2_blend( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	blit_sprite2_blend(surface, x,      y,      sprite2s, index);
	blit_sprite2_blend(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2_blend(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2_blend(surface, x + 12, y + 14, sprite2s, index + 20);
}

// does not clip on left or right edges of surface
void blit_sprite2x2_darken( SDL_Surface *surface, int x, int y, Sprite2_array sprite2s, unsigned int index )
{
	blit_sprite2_darken(surface, x,      y,      sprite2s, index);
	blit_sprite2_darken(surface, x + 12, y,      sprite2s, index + 1);
	blit_sprite2_darken(surface, x,      y + 14, sprite2s, index + 19);
	blit_sprite2_darken(surface, x + 12, y + 14, sprite2s, index + 20);
}


void JE_loadMainShapeTables( const char *shpfile )
{
	FILE *f = dir_fopen_die(data_dir(), shpfile, "rb");

	unsigned int i;
	JE_word shpNumb;
	JE_longint shpPos[14]; // +1 maximum possible
	
	efread(&shpNumb, sizeof(JE_word), 1, f);

	// Detect T2000 by number of shapes / sprite banks, vanilla has 12, T2000 has 13
	tyrian2000detected = (shpNumb != 12);

	for (i = 0; i < shpNumb; ++i)
		efread(&shpPos[i], sizeof(JE_longint), 1, f);

	fseek(f, 0, SEEK_END);
	for (; i < COUNTOF(shpPos); ++i)
		shpPos[i] = ftell(f);

	// fonts, interface, option sprites
	for (i = 0; i < 7; i++)
	{
		fseek(f, shpPos[i], SEEK_SET);
		load_sprites(i, f);
	}

	// player shot sprites
	loadCompShapesArc(&shotShapes[0], "a_shots1.shp");
	// skip shapes in shpfile
	i++;

	// player ship sprites
	loadCompShapesArc(&shipShapes, "a_ships.shp");
	// skip shapes in shpfile
	i++;

	// power-up sprites
	loadCompShapesArc(&iconShapes, "a_icons.shp");
	// skip shapes in shpfile
	i++;

	fseek(f, shpPos[i], SEEK_SET);

	// coins, datacubes, etc sprites
	pickupShapes.size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&pickupShapes, f);
	i++;
	
	// more player shot sprites
	shotShapes[1].size = shpPos[i + 1] - shpPos[i];
	JE_loadCompShapesB(&shotShapes[1], f);
	i++;

	if (tyrian2000detected)
	{
		// extra T2K ships
		shipShapesT2K.size = shpPos[i + 1] - shpPos[i];
		JE_loadCompShapesB(&shipShapesT2K, f);
	}

	fclose(f);

	load_sprites_file(EXTRA_SHAPES, arcdata_dir(), "arcopts.spr");

}

void free_main_shape_tables( void )
{
	for (uint i = 0; i < COUNTOF(sprite_table); ++i)
		free_sprites(i);

	free_sprite2s(&shipShapes);
	free_sprite2s(&iconShapes);
	free_sprite2s(&pickupShapes);
	free_sprite2s(&shotShapes[0]);
	free_sprite2s(&shotShapes[1]);

	if (tyrian2000detected)
		free_sprite2s(&shipShapesT2K);
}
