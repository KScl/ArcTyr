/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2020       Kaito Sinclaire
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
	JE_byte     attack[8], del[8]; /* [1..8] */
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
} JE_ShipType; /* [0..shipnum] */

extern JE_ShipType       *ships; // dynamically allocated
extern size_t num_ships;

//
// Shields
// * Hardcoded, only consists of a power level
#define SHIELD_NUM 10
extern const size_t num_shields;
extern const JE_byte shield_power[SHIELD_NUM + 1];

// Extra data
// Ship selection order
extern JE_byte shiporder_nosecret;
extern JE_byte shiporder_count;
extern JE_byte shiporder[16];

void PlayData_load( void );
void PlayData_free( void );

#endif
