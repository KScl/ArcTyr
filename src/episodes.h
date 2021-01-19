/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  episodes.h
/// \brief Loading and navigating the game's episodes

#ifndef EPISODES_H
#define EPISODES_H

#include "opentyr.h"

/* Episodes and general data */
#define FIRST_LEVEL 1
#define EPISODE_MAX 5
#define EPISODE_AVAILABLE 5

extern JE_longint globalDataLoc;

extern JE_byte initial_episode_num, episodeNum;
extern JE_boolean episodeAvail[EPISODE_MAX];

extern char episode_file[13];
extern char level_file[13];

/* Level Data */
extern JE_longint lvlPos[43];
extern JE_word lvlNum;

/* Enemy Data */
typedef struct
{
	JE_byte     ani;
	JE_byte     tur[3]; /* [1..3] */
	JE_byte     freq[3]; /* [1..3] */
	JE_shortint xmove;
	JE_shortint ymove;
	JE_shortint xaccel;
	JE_shortint yaccel;
	JE_shortint xcaccel;
	JE_shortint ycaccel;
	JE_integer  startx;
	JE_integer  starty;
	JE_shortint startxc;
	JE_shortint startyc;
	JE_byte     armor;
	JE_byte     esize;
	JE_word     egraphic[20];  /* [1..20] */
	JE_byte     explosiontype;
	JE_byte     animate;       /* 0:Not Yet   1:Always   2:When Firing Only */
	JE_byte     shapebank;     /* See LEVELMAK.DOC */
	JE_shortint xrev, yrev;
	JE_word     dgr;
	JE_shortint dlevel;
	JE_shortint dani;
	JE_byte     elaunchfreq;
	JE_word     elaunchtype;
	JE_integer  value;
	JE_word     eenemydie;
} JE_EnemyDatType; /* [0..enemynum] */

// JE_WeaponType, except without the things enemies don't use
typedef struct
{
	JE_byte     multi;
	JE_word     weapani;
	JE_byte     max;
	JE_byte     tx, ty, aim;
	JE_byte     attack[8], del[8]; /* [1..8] */
	JE_shortint sx[8], sy[8]; /* [1..8] */
	JE_shortint bx[8], by[8]; /* [1..8] */
	JE_word     sg[8]; /* [1..8] */
	JE_shortint acceleration[8], accelerationx[8];
	JE_byte     sound;
} JE_EnemyWeaponType;

#define ENEMY_NUM       850
#define T2KENEMY_START 1001 // T2000 has a second enemy bank. Ughhhhhhh
#define T2KENEMY_NUM   1851

extern JE_EnemyDatType    enemyDat[T2KENEMY_NUM + 1];
extern JE_EnemyWeaponType eWeapons[256]; // fixed -- the first 256 shots in episode data / tyrian.hdt

void Episode_init( JE_byte newEpisode );
void Episode_scan( void );

unsigned int Episode_getNext( void );
bool Episode_next( void );

#endif /* EPISODES_H */
