/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  varz.h
/// \brief TODO

#ifndef VARZ_H
#define VARZ_H

#include "loudness.h"
#include "opentyr.h"
#include "player.h"
#include "sprite.h"

#include <stdbool.h>

// instant kill damage
#define DKILL 255

#define ENEMY_INVULNERABLE 1000000

// max dragonwing charge
#define MAXCHARGE 5

// hardcoded shot nums
#define PWPN_TURRET_BIG      1
#define PWPN_TURRET_SMALL    2
#define PWPN_NORTSPARKS      3
#define PWPN_ASTEROID_KILLER 4

#define ENEMY_SHOT_MAX  60 /* 60*/

#define CURRENT_KEY_SPEED 1  /*Keyboard/Joystick movement rate*/

#define MAX_EXPLOSIONS           200
#define MAX_REPEATING_EXPLOSIONS 20
#define MAX_SUPERPIXELS          101

struct JE_SingleEnemyType
{
	JE_byte     fillbyte;
	JE_integer  ex, ey;     /* POSITION */
	JE_shortint exc, eyc;   /* CURRENT SPEED */
	JE_shortint exca, eyca; /* RANDOM ACCELERATION */
	JE_shortint excc, eycc; /* FIXED ACCELERATION WAITTIME */
	JE_shortint exccw, eyccw;
	JE_byte     eshotwait[3], eshotmultipos[3]; /* [1..3] */
	JE_byte     enemycycle;
	JE_byte     ani;
	JE_word     egr[20]; /* [1..20] */
	JE_byte     size;
	JE_byte     linknum;
	JE_byte     aniactive;
	JE_byte     animax;
	JE_byte     aniwhenfire;
	Sprite2_array *sprite2s;
	JE_shortint exrev, eyrev;
	JE_integer  exccadd, eyccadd;
	JE_byte     exccwmax, eyccwmax;
	void       *enemydatofs;
	JE_boolean  edamaged;
	JE_word     enemytype;
	JE_byte     animin;
	JE_word     edgr;
	JE_shortint edlevel;
	JE_shortint edani;
	JE_byte     fill1;
	JE_byte     filter;
	JE_integer  evalue;
	JE_integer  fixedmovey;
	JE_byte     freq[3]; /* [1..3] */
	JE_byte     launchwait;
	JE_word     launchtype;
	JE_byte     launchfreq;
	JE_byte     xaccel;
	JE_byte     yaccel;
	JE_byte     tur[3]; /* [1..3] */
	JE_word     enemydie; /* Enemy created when this one dies */
	JE_boolean  enemyground;
	JE_byte     explonum;
	JE_word     mapoffset;
	JE_boolean  scoreitem;

	JE_boolean  special;
	JE_byte     flagnum;
	JE_boolean  setto;

	JE_byte     iced; /*Duration*/

	JE_byte     launchspecial;

	JE_integer  xminbounce;
	JE_integer  xmaxbounce;
	JE_integer  yminbounce;
	JE_integer  ymaxbounce;
	JE_byte     fill[3]; /* [1..3] */

	// ARC extras (not in data files)
	Uint32      ehealth; // replaces armorleft, armor * 100
	JE_byte     playertotarget; // In two-player games, who to target
};

typedef struct JE_SingleEnemyType JE_MultiEnemyType[125]; /* [1..125] */

typedef JE_word JE_DanCShape[(24 * 28) / 2]; /* [1..(24*28) div 2] */

typedef JE_char JE_CharString[256]; /* [1..256] */

typedef JE_byte JE_Map1Buffer[24 * 28 * 13 * 4]; /* [1..24*28*13*4] */

typedef JE_byte *JE_MapType[300][14]; /* [1..300, 1..14] */
typedef JE_byte *JE_MapType2[600][14]; /* [1..600, 1..14] */
typedef JE_byte *JE_MapType3[600][15]; /* [1..600, 1..15] */

struct JE_EventRecType
{
	JE_word     eventtime;
	JE_byte     eventtype;
	JE_integer  eventdat, eventdat2;
	JE_shortint eventdat3, eventdat5, eventdat6;
	JE_byte     eventdat4;
};

struct JE_MegaDataType1
{
	JE_MapType mainmap;
	struct
	{
		JE_DanCShape sh;
	} shapes[72]; /* [0..71] */
	JE_byte tempdat1;
	/*JE_DanCShape filler;*/
};

struct JE_MegaDataType2
{
	JE_MapType2 mainmap;
	struct
	{
		JE_byte nothing[3]; /* [1..3] */
		JE_byte fill;
		JE_DanCShape sh;
	} shapes[71]; /* [0..70] */
	JE_byte tempdat2;
};

struct JE_MegaDataType3
{
	JE_MapType3 mainmap;
	struct
	{
		JE_byte nothing[3]; /* [1..3] */
		JE_byte fill;
		JE_DanCShape sh;
	} shapes[70]; /* [0..69] */
	JE_byte tempdat3;
};

typedef JE_byte JE_EnemyAvailType[125]; /* [1..125] */

typedef struct {
	JE_integer sx, sy;
	JE_integer sxm, sym;
	JE_shortint sxc, syc;
	JE_byte tx, ty;
	JE_word sgr;
	JE_byte sdmg;
	JE_byte duration;
	JE_word animate;
	JE_word animax;
	JE_byte fill[12];
} EnemyShotType;

typedef struct {
	unsigned int ttl;
	signed int x, y;
	signed int delta_x, delta_y;
	bool fixed_position;
	bool follow_player;
	unsigned int sprite;
} explosion_type;

typedef struct {
	unsigned int delay;
	unsigned int ttl;
	unsigned int x, y;
	bool big;
} rep_explosion_type;

typedef struct {
	unsigned int x, y, z;
	signed int delta_x, delta_y;
	Uint8 color;
} superpixel_type;

extern const JE_byte randomEnemyLaunchSounds[3];
extern JE_byte lvlFileNum;
extern JE_word maxEvent, eventLoc;
extern JE_word tempBackMove, explodeMove;
extern JE_byte levelEnd;
extern JE_word deathExplodeSfxWait;
extern JE_shortint levelEndWarp;
extern JE_boolean endLevel, reallyEndLevel, waitToEndLevel, playerEndLevel, normalBonusLevelCurrent, bonusLevelCurrent, smallEnemyAdjust, readyToEndLevel;
extern JE_byte newPL[10];
extern JE_word returnLoc;
extern JE_boolean returnActive;
extern JE_boolean debug;
extern Uint32 debugTime, lastDebugTime;
extern JE_longint debugHistCount;
extern JE_real debugHist;
extern JE_word curLoc;
extern JE_boolean gameNotOverYet, gameLoaded, enemyStillExploding;
extern uint exitGameTic, attractTic;
extern JE_word totalEnemy;
extern JE_word enemyKilled[2];
extern struct JE_MegaDataType1 megaData1;
extern struct JE_MegaDataType2 megaData2;
extern struct JE_MegaDataType3 megaData3;
extern JE_byte secretLevelDisplayTime;
extern JE_boolean goingToBonusLevel;

extern JE_byte soundQueue[STANDARD_SFX_CHANNELS];
extern JE_boolean enemyContinualDamage;
extern JE_boolean enemiesActive;
extern JE_boolean forceEvents;
extern JE_boolean stopBackgrounds;
extern JE_byte stopBackgroundNum;
extern JE_byte damageRate;
extern JE_boolean background3x1;
extern JE_boolean background3x1b;
extern JE_boolean levelTimer;
extern JE_word levelTimerCountdown;
extern JE_word levelTimerJumpTo;
extern JE_boolean randomExplosions;
extern JE_boolean globalFlags[10];
extern JE_byte levelSong;
extern JE_word mapOrigin, mapPNum;
extern JE_byte mapPlanet[5], mapSection[5];
extern JE_MultiEnemyType enemy;
extern JE_EnemyAvailType enemyAvail;
extern JE_word enemyOffset;
extern JE_word enemyOnScreen;
extern JE_byte enemyShapeTables[4];
extern JE_word superEnemy254Jump;
extern explosion_type explosions[MAX_EXPLOSIONS];
extern JE_integer explosionFollowAmountX, explosionFollowAmountY;
extern JE_boolean enemyShotAvail[ENEMY_SHOT_MAX];
extern EnemyShotType enemyShot[ENEMY_SHOT_MAX];
extern JE_byte astralDuration;
extern JE_boolean allPlayersGone;
extern const uint shadowYDist;
extern JE_byte chargeWait, chargeLevel, chargeGr, chargeGrWait;
extern JE_word neat;
extern rep_explosion_type rep_explosions[MAX_REPEATING_EXPLOSIONS];
extern superpixel_type superpixels[MAX_SUPERPIXELS];
extern unsigned int last_superpixel;
extern JE_byte temp, temp2, temp3;
extern JE_word tempX, tempY;
extern JE_word tempW;
extern JE_word x, y;
extern JE_integer b;
extern JE_byte **BKwrap1to, **BKwrap2to, **BKwrap3to, **BKwrap1, **BKwrap2, **BKwrap3;

static const int hud_sidekickX[2] = {   1, 293 };
static const int hud_sidekickY[2] = {  69,  88 };

void JE_getShipInfo( void );

void JE_updateOption( Player *this_player, uint i );
void JE_updateAllOptions( void );

void JE_specialComplete( JE_byte playernum, JE_byte specialType, uint shot_i, JE_byte twiddlePower );
void JE_doSpecialShot( JE_byte playernum );

void JE_setupExplosion( signed int x, signed int y, signed int delta_y, unsigned int type, bool fixed_position, bool follow_player );
void JE_setupExplosionLarge( JE_boolean enemyground, JE_byte explonum, JE_integer x, JE_integer y );

void JE_drawShield( void );
void JE_drawArmor( void );

/*SuperPixels*/
void JE_doSP( JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color );
void JE_drawSP( void );

extern JE_byte globalFlare;
extern JE_shortint globalFlareFilter;

#endif /* VARZ_H */

