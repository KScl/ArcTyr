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
#ifndef LOUDNESS_H
#define LOUDNESS_H

#include "opentyr.h"

#include "lib/opl.h"

#include "SDL.h"

#define SFX_CHANNELS 16
#define STANDARD_SFX_CHANNELS 8 // non-priority channels used for regular game audio

#if defined(TARGET_GP2X) || defined(TARGET_DINGUX)
#define OUTPUT_QUALITY 2  // 22 kHz
#else
#define OUTPUT_QUALITY 4  // 44 kHz
#endif

#define SAMPLE_SCALING OUTPUT_QUALITY
#define SAMPLE_TYPE Bit16s
#define BYTES_PER_SAMPLE 2

extern float music_volume, sample_volume;

extern unsigned int song_playing;

extern bool audio_disabled, music_disabled, samples_disabled;

bool init_audio( void );
void deinit_audio( void );

void load_music( void );
void play_song_once( unsigned int song_num );
void play_song( unsigned int song_num );
void restart_song( void );
void stop_song( void );
void fade_song( void );

void set_volume( unsigned int music, unsigned int sample );

void JE_multiSamplePlay(JE_byte *buffer, JE_word size, JE_byte chan, JE_byte vol);

// musmast
#define MUSIC_NUM 41

enum
{
	SONG_BUY         = 2,
	SONG_ENDGAME1    = 7,
	SONG_CREDITS     = 8,
	SONG_LEVELEND    = 9,
	SONG_GAMEOVER    = 10,
	SONG_EPISODEEND  = 18,
	SONG_MAPVIEW     = 19,
	SONG_FAILURE     = 21,
	SONG_NEXTEPISODE = 26,
	SONG_TITLE       = 29,
	SONG_BONUSLEVEL  = 30,
	SONG_ZANAC       = 31,
};

#endif /* LOUDNESS_H */

