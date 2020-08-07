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
#ifndef NORTSONG_H
#define NORTSONG_H

#include "opentyr.h"

#include "SDL.h"

// sndmast
#define SAMPLE_COUNT 64

#define SFXPRIORITY 8 // channel number for priority sfx
// SFXPRIORITY  : Timer clicks
// SFXPRIORITY+1: Timer warning
// SFXPRIORITY+2: Susan's voice lines
//
// SFXPRIORITY+4: Dragonwing link sounds
//
// SFXPRIORITY+6: Inserting coin
// SFXPRIORITY+7: Misc display sounds

extern Uint32 target, target2;

extern JE_word frameCount, frameCount2, frameCountMax;

extern JE_byte *digiFx[SAMPLE_COUNT];
extern JE_word fxSize[SAMPLE_COUNT];

extern JE_word tyrMusicVolume, fxVolume;
extern JE_word fxPlayVol;
extern JE_word tempVolume;

extern JE_word speed;

extern float jasondelay;

void setdelay( JE_byte delay );
void setjasondelay( int delay );
void setjasondelay2( int delay );
int delaycount( void );
int delaycount2( void );

void wait_delay( void );
void service_wait_delay( void );
void wait_delayorinput( void );

void JE_resetTimerInt( void );
void JE_setTimerInt( void );

void JE_calcFXVol( void );
void JE_changeVolume( JE_word *music, int music_delta, JE_word *sample, int sample_delta );

void JE_loadSndFile( const char *effects_sndfile, const char *voices_sndfile );
void JE_playSampleNum( JE_byte samplenum );
void JE_playSampleNumOnChannel( JE_byte samplenum, JE_byte chan );

// sndmast
enum
{
	S_NONE             =  0,
	S_WEAPON_1         =  1,
	S_WEAPON_2         =  2,
	S_ENEMY_HIT        =  3,
	S_EXPLOSION_4      =  4,
	S_WEAPON_5         =  5,
	S_WEAPON_6         =  6,
	S_WEAPON_7         =  7,
	S_SELECT           =  8,
	S_EXPLOSION_8      =  8,
	S_EXPLOSION_9      =  9,
	S_WEAPON_10        = 10,
	S_EXPLOSION_11     = 11,
	S_EXPLOSION_12     = 12,
	S_WEAPON_13        = 13,
	S_WEAPON_14        = 14,
	S_WEAPON_15        = 15,
	S_SPRING           = 16,
	S_WARNING          = 17,
	S_ITEM             = 18,
	S_HULL_HIT         = 19,
	S_MACHINE_GUN      = 20,
	S_SOUL_OF_ZINGLON  = 21,
	S_EXPLOSION_22     = 22,
	S_CLINK            = 23,
	S_CLICK            = 24,
	S_WEAPON_25        = 25,
	S_WEAPON_26        = 26,
	S_SHIELD_HIT       = 27,
	S_CURSOR           = 28,
	S_POWERUP          = 29,
	S_WEAPON_30        = 30, // T2000
	S_WEAPON_31        = 31, // T2000
	V_CLEARED_PLATFORM = 32,  // "Cleared enemy platform."
	V_BOSS             = 33,  // "Large enemy approaching."
	V_ENEMIES          = 34,  // "Enemies ahead."
	V_GOOD_LUCK        = 35,  // "Good luck."
	V_LEVEL_END        = 36,  // "Level completed."
	V_DANGER           = 37,  // "Danger."
	V_SPIKES           = 38,  // "Warning: spikes ahead."
	V_DATA_CUBE        = 39,  // "Data acquired."
	V_ACCELERATE       = 40,  // "Unexplained speed increase."
	S_EXPLOSION_41     = 41,
	S_WEAPON_42        = 42,
};

#endif /* NORTSONG_H */
