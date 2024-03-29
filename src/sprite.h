/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  sprite.h
/// \brief Loading and displaying game sprites

#ifndef SPRITE_H
#define SPRITE_H

#include "opentyr.h"

#include "SDL.h"
#include <assert.h>
#include <stdio.h>

#define FONT_SHAPES       0
#define SMALL_FONT_SHAPES 1
#define TINY_FONT         2
#define PLANET_SHAPES     3
#define FACE_SHAPES       4
#define OPTION_SHAPES     5 /*Also contains help shapes*/
#define WEAPON_SHAPES     6
#define EXTRA_SHAPES      7 /*Used for TAV sidekicks, ending pics*/

#define SPRITE_TABLES_MAX        8
#define SPRITES_PER_TABLE_MAX  152 /* T2000 support: 2.1 is 151 */

typedef struct
{
	Uint16 width, height;
	Uint16 size;
	Uint8 *data;
}
Sprite;

typedef struct
{
	unsigned int count;
	Sprite sprite[SPRITES_PER_TABLE_MAX];
}
Sprite_array;

extern Sprite_array sprite_table[SPRITE_TABLES_MAX];

static inline Sprite *sprite( unsigned int table, unsigned int index )
{
	assert(table < COUNTOF(sprite_table));
	assert(index < COUNTOF(sprite_table->sprite));
	return &sprite_table[table].sprite[index];
}

static inline bool sprite_exists( unsigned int table, unsigned int index )
{
	return (sprite(table, index)->data != NULL);
}
static inline Uint16 get_sprite_width( unsigned int table, unsigned int index )
{
	return (sprite_exists(table, index) ? sprite(table, index)->width : 0);
}
static inline Uint16 get_sprite_height( unsigned int table, unsigned int index )
{
	return (sprite_exists(table, index) ? sprite(table, index)->height : 0);
}

void load_sprites_file( unsigned table, const char *dir, const char *filename );
void load_sprites( unsigned int table, FILE *f );
void free_sprites( unsigned int table );

void blit_sprite( SDL_Surface *, int x, int y, unsigned int table, unsigned int index ); // JE_newDrawCShapeNum
void blit_sprite_blend( SDL_Surface *, int x, int y, unsigned int table, unsigned int index ); // JE_newDrawCShapeTrick
void blit_sprite_hv_unsafe( SDL_Surface *, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value ); // JE_newDrawCShapeBright
void blit_sprite_hv( SDL_Surface *, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value ); // JE_newDrawCShapeAdjust
void blit_sprite_hv_blend( SDL_Surface *, int x, int y, unsigned int table, unsigned int index, Uint8 hue, Sint8 value ); // JE_newDrawCShapeModify
void blit_sprite_dark( SDL_Surface *, int x, int y, unsigned int table, unsigned int index, bool black ); // JE_newDrawCShapeDarken, JE_newDrawCShapeShadow

typedef struct
{
	unsigned int size;
	Uint8 *data;
}
Sprite2_array;

extern Sprite2_array shotShapes[2];
extern Sprite2_array iconShapes;
extern Sprite2_array pickupShapes;
extern Sprite2_array shipShapes, shipShapesT2K;
extern Sprite2_array eShapes[4];
extern Sprite2_array shapes6;

void JE_loadCompShapes( Sprite2_array *, JE_char s );
void JE_loadCompShapesB( Sprite2_array *, FILE *f );
void loadCompShapesArc( Sprite2_array *, const char *filename );

void free_sprite2s( Sprite2_array * );

void blit_sprite2( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index );
void blit_sprite2_blend( SDL_Surface *,  int x, int y, Sprite2_array, unsigned int index );
void blit_sprite2_darken( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index );
void blit_sprite2_filter( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index, Uint8 filter );

void blit_sprite2x2( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index );
void blit_sprite2x2_blend( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index );
void blit_sprite2x2_darken( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index );

void JE_loadMainShapeTables( const char *shpfile );
void free_main_shape_tables( void );

#endif // SPRITE_H

