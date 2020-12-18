/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  playdata.h
/// \brief Player specific game data, and functions for loading such

#ifndef PLAYDATA_H
#define PLAYDATA_H

#include "opentyr.h"

//
// Player and enemy shots
//
typedef struct
{
	JE_byte     shotrepeat;
	JE_byte     multi;
	JE_word     weapani;
	JE_byte     max;
	JE_byte     tx, ty, aim;
	JE_word     dmgAmount[8], dmgChain[8];
	JE_byte     dmgIce[8];
	JE_boolean  dmgInfinite[8];
	JE_byte     del[8]; /* [1..8] */
	JE_shortint sx[8], sy[8]; /* [1..8] */
	JE_shortint bx[8], by[8]; /* [1..8] */
	JE_word     sg[8]; /* [1..8] */
	JE_shortint acceleration[8], accelerationx[8];
	JE_byte     circlesize[8];
	JE_byte     sound;
	JE_byte     trail;
	JE_byte     shipblastfilter;
} JE_WeaponType;

extern JE_WeaponType     *pWeapons; // dynamically allocated
extern size_t num_pWeapons;

//
// Port weapons (collections of firing modes)
//
typedef struct
{
	char    name[31]; // string[30]
	JE_byte opnum;

	// note: replaces op[2][11] in vanilla, since second fire modes aren't used
	JE_word normalOp[11]; // main weapon
	JE_word chargeOp[5]; // dragonwing charged
	JE_word aimedOp[6]; // dragonwing aimed
	JE_byte dwSidekick[3]; // dragonwing sidekick given

	JE_word cost;
	JE_word itemgraphic;
	JE_word poweruse;
} JE_WeaponPortType;

extern JE_WeaponPortType *weaponPort; // dynamically allocated
extern size_t num_ports;

//
// Special weapons / Twiddles
//
typedef struct
{
	char    name[31]; /* string [30] */
	JE_word itemgraphic;
	JE_byte pwr;
	JE_byte stype;
	JE_word wpn;
	JE_byte extradata;
} JE_SpecialType; /* [0..specialnum] */

extern JE_SpecialType    *special; // dynamically allocated
extern size_t num_specials;

//
// Sidekicks / Options
//
typedef struct
{
	char        name[31]; /* string [30] */
	JE_byte     pwr;
	JE_word     itemgraphic;
	JE_word     cost;
	JE_byte     tr, option;
	JE_shortint opspd;
	JE_byte     ani;
	JE_word     gr[20]; /* [1..20] */
	JE_byte     wport;
	JE_word     wpnum;
	JE_byte     ammo;
	JE_boolean  stop;
	JE_byte     icongr;
} JE_OptionType;

extern JE_OptionType     *options; // dynamically allocated
extern size_t num_options;

//
// Player Ships
//
typedef struct
{
	char        name[31]; /* string [30] */
	JE_word     shipgraphic;
	JE_word     itemgraphic;
	JE_byte     ani;
	JE_shortint spd;
	JE_byte     dmg;
	JE_word     cost;
	JE_byte     bigshipgraphic;

	// ARC data
	JE_word     special_weapons[2];
	JE_word     port_weapons[5];
	JE_byte     sidekick_start[2];

	JE_byte     numTwiddles;
	JE_byte     twiddles[10][8];

	// automatically populated with info related to ship_select
	struct
	{
		bool present;
		JE_byte x, y;
	} locationinmenu;
	struct
	{
		bool present;
		JE_byte x;
	} locationingame;
} JE_ShipType; /* [0..shipnum] */

extern JE_ShipType       *ships; // dynamically allocated
extern size_t num_ships;

//
// Shields
// * Hardcoded, only consists of a power level
#define SHIELD_NUM 10
extern const size_t num_shields;
extern const JE_byte shield_power[SHIELD_NUM + 1];

//
// Hints
//
typedef struct
{
	JE_byte reference; // What ship (or episode) is the hint referencing?
	JE_byte weight; // How likely is the hint to show up?
	char text[7][61];
} hint_type_t;

extern hint_type_t *hints;
extern size_t num_hints;

// Extra data
// Ship selection order
#define MAX_SHIP_SELECT 16
typedef struct
{
	JE_byte ship;
	JE_byte code[16];
} ship_select_code_t;

#define SHIP_SELECT_TOP 0
#define SHIP_SELECT_BOTTOM 1
#define SHIP_SELECT_CONTINUE 2
extern JE_byte ship_select[3][MAX_SHIP_SELECT]; // [row][index] -- top, bottom, continue screen
extern JE_byte num_ship_select[3];
extern ship_select_code_t secret_ship_codes[8];
extern JE_byte num_secret_ship_codes;

void PlayData_load( void );
void PlayData_free( void );

#endif
