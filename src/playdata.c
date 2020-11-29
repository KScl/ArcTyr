/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020     Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  playdata.c
/// \brief Player specific game data, and functions for loading such

#include "arcade.h"
#include "episodes.h"
#include "file.h"
#include "opentyr.h"
#include "playdata.h"

// Technically data ... condensed down to the most basic form
// Shields used to be separate items, but we sure don't use them like that anymore
const size_t num_shields = SHIELD_NUM;
const JE_byte shield_power[SHIELD_NUM + 1] = {0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

// Player shots
size_t         num_pWeapons = 0;
JE_WeaponType *pWeapons     = NULL; // dynamically allocated

// Port weapons
size_t             num_ports  = 0;
JE_WeaponPortType *weaponPort = NULL; // dynamically allocated

// Options / Sidekicks
size_t         num_options  = 0;
JE_OptionType *options      = NULL; // dynamically allocated

// Specials / Twiddles
size_t          num_specials = 0;
JE_SpecialType *special      = NULL; // dynamically allocated

// Ships
size_t       num_ships  = 0;
JE_ShipType *ships      = NULL; // dynamically allocated

// Ship selection organization
JE_byte shiporder_nosecret;
JE_byte shiporder_count;
JE_byte shiporder[16];         // [index] -> ship#
JE_byte reverse_shiporder[16]; // [ship#] -> index


//
// Loading individual player item data files
//

static void _loadPWeapons( const char *dataFile )
{
	FILE *f = NULL;
	size_t shotSize;
	JE_byte tmp_b;
	Uint32 tmp_int[8];

	f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	efread(&num_pWeapons, sizeof(JE_word), 1, f);
	shotSize = sizeof(JE_WeaponType) * (num_pWeapons + 1);
	pWeapons = realloc(pWeapons, shotSize);
	memset(pWeapons, 0, shotSize);

	for (size_t i = 0; i < num_pWeapons + 1; ++i)
	{
		// tx and ty are not used for player shots
		efread(&pWeapons[i].shotrepeat,      sizeof(JE_byte), 1, f);
		efread(&pWeapons[i].weapani,         sizeof(JE_word), 1, f);
		efread(&pWeapons[i].aim,             sizeof(JE_byte), 1, f);

		// max top four bits, multi bottom four bits
		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		pWeapons[i].multi = (tmp_b & 0x0F);
		pWeapons[i].max   = (tmp_b & 0xF0) >> 4;

		if (pWeapons[i].max > 0)
		{
			efread(tmp_int,                      sizeof(Uint32),      pWeapons[i].max, f);
			for (size_t j = 0; j < pWeapons[i].max; ++j)
			{
				pWeapons[i].dmgAmount[j] = tmp_int[j] & 0x00003FFF;
				pWeapons[i].dmgIce[j] = (tmp_int[j] & 0x0003C000) >> 14;
				pWeapons[i].dmgChain[j] = (tmp_int[j] & 0x7FFC0000) >> 18;
				pWeapons[i].dmgInfinite[j] = (tmp_int[j] & 0x80000000) ? 1 : 0;
			}
			efread(&pWeapons[i].del,             sizeof(JE_byte),     pWeapons[i].max, f);
			efread(&pWeapons[i].sx,              sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].sy,              sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].bx,              sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].by,              sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].sg,              sizeof(JE_word),     pWeapons[i].max, f);
			efread(&pWeapons[i].acceleration,    sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].accelerationx,   sizeof(JE_shortint), pWeapons[i].max, f);
			efread(&pWeapons[i].circlesize,      sizeof(JE_byte),     pWeapons[i].max, f);
		}

		efread(&pWeapons[i].sound,           sizeof(JE_byte), 1, f);
		efread(&pWeapons[i].trail,           sizeof(JE_byte), 1, f);
		efread(&pWeapons[i].shipblastfilter, sizeof(JE_byte), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: shots array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

static void _loadPorts( const char *dataFile )
{
	FILE *f = NULL;
	size_t portSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	efread(&num_ports, sizeof(JE_byte), 1, f);
	portSize = sizeof(JE_WeaponPortType) * (num_ports + 1);
	weaponPort = realloc(weaponPort, portSize);
	memset(weaponPort, 0, portSize);

	for (size_t i = 0; i < num_ports + 1; ++i)
	{
		efread(&tmp_b,                     sizeof(JE_byte), 1, f);
		efread(&weaponPort[i].name,        1, tmp_b, f);
		weaponPort[i].name[tmp_b] = 0;

		efread(&tmp_b,                     sizeof(JE_byte), 1, f);
		efread(&weaponPort[i].normalOp,    sizeof(JE_word), 11, f);
		if (tmp_b == 2)
		{			
			efread(&weaponPort[i].chargeOp,    sizeof(JE_word), 5, f);
			efread(&weaponPort[i].aimedOp,     sizeof(JE_word), 6, f);
			efread(&weaponPort[i].dwSidekick,  sizeof(JE_byte), 3, f);
		}
		efread(&weaponPort[i].cost,        sizeof(JE_word), 1, f);
		efread(&weaponPort[i].itemgraphic, sizeof(JE_word), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: ports array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

static void _loadSpecials( const char *dataFile )
{
	FILE *f = NULL;
	size_t specSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	efread(&num_specials, sizeof(JE_byte), 1, f);
	specSize = sizeof(JE_SpecialType) * (num_specials + 1);
	special = realloc(special, specSize);
	memset(special, 0, specSize);

	for (size_t i = 0; i < num_specials + 1; ++i)
	{
		fseek(f, 1, SEEK_CUR); // skip string length
		efread(&special[i].name,        1, 30, f);
		special[i].name[30] = '\0';

		efread(&special[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&special[i].pwr,         sizeof(JE_byte), 1, f);
		efread(&special[i].stype,       sizeof(JE_byte), 1, f);
		efread(&special[i].wpn,         sizeof(JE_word), 1, f);
		efread(&special[i].extradata,   sizeof(JE_byte), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: specials array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

static void _loadShips( const char *dataFile )
{
	FILE *f = NULL;
	size_t i, shipSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	efread(&num_ships, sizeof(JE_byte), 1, f);
	shipSize = sizeof(JE_ShipType) * (num_ships + 1);
	ships = realloc(ships, shipSize);
	memset(ships, 0, shipSize);

	// Order of display on Ship Select
	efread(&shiporder_count, sizeof(JE_byte), 1, f);
	memset(shiporder, 0, sizeof(shiporder));
	memset(reverse_shiporder, 0xFF, sizeof(reverse_shiporder));

	for (i = 0; i < shiporder_count; ++i)
	{
		efread(&shiporder[i], sizeof(JE_byte), 1, f);
		if ((shiporder[i] & 0x40) && !tyrian2000detected)
		{ // T2000 only ship?
			--i;
			--shiporder_count;
			continue;
		}
		if (!(shiporder[i] & 0x80))
			++shiporder_nosecret;
		shiporder[i] &= 0x3F;
		reverse_shiporder[shiporder[i]] = i;
	}

	efread(&tmp_b, sizeof(JE_byte), 1, f);
	if (tmp_b != ';')
	{
		fprintf(stderr, "error: ships array is bad at <shiporder>: got %d\n", tmp_b);
		JE_tyrianHalt(1);
	}

	for (i = 0; i < num_ships + 1; ++i)
	{
		fseek(f, 1, SEEK_CUR); 
		efread(&ships[i].name,           1, 30, f);
		ships[i].name[30] = '\0';
		//printf("%s\n", ships[i].name);
		efread(&ships[i].shipgraphic,    sizeof(JE_word), 1, f);
		efread(&ships[i].itemgraphic,    sizeof(JE_word), 1, f);
		efread(&ships[i].ani,            sizeof(JE_byte), 1, f);
		efread(&ships[i].spd,            sizeof(JE_shortint), 1, f);
		efread(&ships[i].dmg,            sizeof(JE_byte), 1, f);
		efread(&ships[i].cost,           sizeof(JE_word), 1, f);
		efread(&ships[i].bigshipgraphic, sizeof(JE_byte), 1, f);

		for (int j = 0; j < 2; ++j)
			efread(&ships[i].special_weapons[j], sizeof(JE_word), 1, f);
		for (int j = 0; j < 5; ++j)
			efread(&ships[i].port_weapons[j],    sizeof(JE_word), 1, f);
		for (int j = 0; j < 2; ++j)
			efread(&ships[i].sidekick_start[j],  sizeof(JE_byte), 1, f);

		efread(&ships[i].numTwiddles,            sizeof(JE_byte), 1, f);
		for (int j = 0; j < ships[i].numTwiddles; ++j)
			for (int k = 0; k < 8; ++k)
				efread(&ships[i].twiddles[j][k], sizeof(JE_byte), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: ships array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

static void _loadOptions( const char *dataFile )
{
	FILE *f = NULL;
	size_t optSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	efread(&num_options, sizeof(JE_byte), 1, f);
	optSize = sizeof(JE_OptionType) * (num_options + 1);
	options = realloc(options, optSize);
	memset(options, 0, optSize);

	for (size_t i = 0; i < num_options + 1; ++i)
	{
		fseek(f, 1, SEEK_CUR); 
		efread(&options[i].name,        1, 30, f);
		options[i].name[30] = '\0';

		efread(&options[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&options[i].cost,        sizeof(JE_word), 1, f);
		efread(&options[i].pwr,         sizeof(JE_byte), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		options[i].tr     = (tmp_b & 0x0F);
		options[i].option = (tmp_b & 0xF0) >> 4;

		efread(&options[i].ani,         sizeof(JE_byte), 1, f);
		if (options[i].ani)
			efread(&options[i].gr,          sizeof(JE_word), options[i].ani, f);

		efread(&options[i].wpnum,       sizeof(JE_word), 1, f);
		efread(&options[i].ammo,        sizeof(JE_byte), 1, f);
		efread(&options[i].icongr,      sizeof(JE_byte), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: options array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

void _loadStaticEnemies( const char *dataFile )
{
	JE_byte tmp_b;
	FILE *f = dir_fopen_die(arcdata_dir(), dataFile, "rb");

	getc(f); // always 100
	for (int i = 900; i <= 999; ++i)
	{
		efread(&enemyDat[i].ani,           sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].tur,           sizeof(JE_byte), 3, f);
		efread(&enemyDat[i].freq,          sizeof(JE_byte), 3, f);
		efread(&enemyDat[i].xmove,         sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].ymove,         sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].xaccel,        sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].yaccel,        sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].xcaccel,       sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].ycaccel,       sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].startx,        sizeof(JE_integer), 1, f);
		efread(&enemyDat[i].starty,        sizeof(JE_integer), 1, f);
		efread(&enemyDat[i].startxc,       sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].startyc,       sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].armor,         sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].esize,         sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].egraphic,      sizeof(JE_word), 20, f);
		efread(&enemyDat[i].explosiontype, sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].animate,       sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].shapebank,     sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].xrev,          sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].yrev,          sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].dgr,           sizeof(JE_word), 1, f);
		efread(&enemyDat[i].dlevel,        sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].dani,          sizeof(JE_shortint), 1, f);
		efread(&enemyDat[i].elaunchfreq,   sizeof(JE_byte), 1, f);
		efread(&enemyDat[i].elaunchtype,   sizeof(JE_word), 1, f);
		efread(&enemyDat[i].value,         sizeof(JE_integer), 1, f);
		efread(&enemyDat[i].eenemydie,     sizeof(JE_word), 1, f);

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: enemies array is bad at %d: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

void PlayData_load( void )
{
	// Unlike the original game, these are loaded once on startup and never again
	ARC_IdentifyPrint("");

	_loadPWeapons("arcshot.dta");
	sprintf(tmpBuf.l, "arcshot.dta: load OK (%zu items)", num_pWeapons);
	ARC_IdentifyPrint(tmpBuf.l);

	_loadPorts("arcport.dta");
	sprintf(tmpBuf.l, "arcport.dta: load OK (%zu items)", num_ports);
	ARC_IdentifyPrint(tmpBuf.l);

	_loadSpecials("arcspec.dta");
	sprintf(tmpBuf.l, "arcspec.dta: load OK (%zu items)", num_specials);
	ARC_IdentifyPrint(tmpBuf.l);

	_loadShips("arcship.dta");
	sprintf(tmpBuf.l, "arcship.dta: load OK (%zu items)", num_ships);
	ARC_IdentifyPrint(tmpBuf.l);

	_loadOptions("arcopt.dta");
	sprintf(tmpBuf.l, "arcopt.dta: load OK (%zu items)", num_options);
	ARC_IdentifyPrint(tmpBuf.l);

	_loadStaticEnemies("enemies.dta");
}

void PlayData_free( void )
{
	if (pWeapons)   free(pWeapons);
	if (weaponPort) free(weaponPort);
	if (options)    free(options);
	if (special)    free(special);
	if (ships)      free(ships);
}
