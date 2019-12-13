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
#include "arcade.h"
#include "config.h"
#include "episodes.h"
#include "file.h"
#include "lvllib.h"
#include "lvlmast.h"
#include "opentyr.h"
#include "varz.h"

/* MAIN Weapons Data */
JE_WeaponType  eWeapons[256];
JE_WeaponType *pWeapons = NULL; // dynamically allocated

size_t num_pWeapons = 0;

/* Items */
JE_ShieldType      shields[SHIELD_NUM + 1];
JE_WeaponPortType *weaponPort = NULL; // dynamically allocated
JE_ShipType       *ships = NULL; // dynamically allocated
JE_OptionType     *options = NULL; // dynamically allocated
JE_SpecialType    *special = NULL; // dynamically allocated

size_t num_ports    = 0;
size_t num_ships    = 0;
size_t num_options  = 0;
size_t num_specials = 0;

/* Enemy data */
JE_EnemyDatType *enemyDat;

/* EPISODE variables */
JE_byte    initial_episode_num, episodeNum = 0;
JE_boolean episodeAvail[EPISODE_MAX]; /* [1..episodemax] */
char       episode_file[13], cube_file[13];

JE_longint episode1DataLoc;

/* Tells the game whether the level currently loaded is a bonus level. */
JE_boolean bonusLevel;

/* Tells if the game jumped back to Episode 1 */
JE_boolean jumpBackToEpisode1;

//
// Arcade data
//

void ADTA_loadShots( void )
{
	FILE *f = NULL;
	size_t shotSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), "arcshot.dta", "rb");

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

		// bits set for repeated data, 1 for repeated, 0 for not
		// this is for all multi-shot vars except sx and sy, which
		// are the most likely to vary
		efread(&tmp_b,                       sizeof(JE_byte), 1, f);

		if (pWeapons[i].max > 0)
		{
			efread(&pWeapons[i].attack,          sizeof(JE_byte),     (tmp_b & 0x01) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].del,             sizeof(JE_byte),     (tmp_b & 0x02) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].sx,              sizeof(JE_shortint),                      pWeapons[i].max, f);
			efread(&pWeapons[i].sy,              sizeof(JE_shortint),                      pWeapons[i].max, f);
			efread(&pWeapons[i].bx,              sizeof(JE_shortint), (tmp_b & 0x04) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].by,              sizeof(JE_shortint), (tmp_b & 0x08) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].sg,              sizeof(JE_word),     (tmp_b & 0x10) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].acceleration,    sizeof(JE_shortint), (tmp_b & 0x20) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].accelerationx,   sizeof(JE_shortint), (tmp_b & 0x40) ? 1 : pWeapons[i].max, f);
			efread(&pWeapons[i].circlesize,      sizeof(JE_byte),     (tmp_b & 0x80) ? 1 : pWeapons[i].max, f);
		}
		if (pWeapons[i].max > 1 && tmp_b)
		{
			// if bit set for repeated data, fill all remaining values with the first entry
			if (tmp_b & 0x01) 
				memset(&pWeapons[i].attack,        pWeapons[i].attack[0],        sizeof(JE_byte) * 8);
			if (tmp_b & 0x02)
				memset(&pWeapons[i].del,           pWeapons[i].del[0],           sizeof(JE_byte) * 8);
			if (tmp_b & 0x04)
				memset(&pWeapons[i].bx,            pWeapons[i].bx[0],            sizeof(JE_shortint) * 8);
			if (tmp_b & 0x08)
				memset(&pWeapons[i].by,            pWeapons[i].by[0],            sizeof(JE_shortint) * 8);
			if (tmp_b & 0x10)
			{
				// the odd one out -- can't just do memset for something that's a word
				pWeapons[i].sg[1] = pWeapons[i].sg[0];
				memcpy(&pWeapons[i].sg[2], &pWeapons[i].sg[0], sizeof(JE_word) * 2);
				memcpy(&pWeapons[i].sg[4], &pWeapons[i].sg[0], sizeof(JE_word) * 4);
			}
			if (tmp_b & 0x20)
				memset(&pWeapons[i].acceleration,  pWeapons[i].acceleration[0],  sizeof(JE_shortint) * 8);
			if (tmp_b & 0x40)
				memset(&pWeapons[i].accelerationx, pWeapons[i].accelerationx[0], sizeof(JE_shortint) * 8);
			if (tmp_b & 0x80)
				memset(&pWeapons[i].circlesize,    pWeapons[i].circlesize[0],    sizeof(JE_byte) * 8);
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

void ADTA_loadPorts( void )
{
	FILE *f = NULL;
	size_t portSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), "arcport.dta", "rb");

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

void ADTA_loadSpecials( void )
{
	FILE *f = NULL;
	size_t specSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), "arcspec.dta", "rb");

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

		efread(&tmp_b,                       sizeof(JE_byte), 1, f);
		if (tmp_b != ';')
		{
			fprintf(stderr, "error: ships array is bad at %zu: got %d\n", i, tmp_b);
			JE_tyrianHalt(1);
		}
	}
}

void ADTA_loadShips( void )
{
	FILE *f = NULL;
	size_t shipSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), "arcship.dta", "rb");

	efread(&num_ships, sizeof(JE_byte), 1, f);
	shipSize = sizeof(JE_ShipType) * (num_ships + 1);
	ships = realloc(ships, shipSize);
	memset(ships, 0, shipSize);

	for (size_t i = 0; i < num_ships + 1; ++i)
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
		efread(&ships[i].numTwiddles,            sizeof(JE_byte), 1, f);
		for (int j = 0; j < 5; ++j)
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

void ADTA_loadOptions( void )
{
	FILE *f = NULL;
	size_t optSize;
	JE_byte tmp_b;

	f = dir_fopen_die(arcdata_dir(), "arcopt.dta", "rb");

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

void ADTA_loadShields( void )
{
	// We don't even load a file for this
	char n[31] = "Shield Power Level 1          ";

	memset(shields, 0, sizeof(shields));

	for (size_t i = 1; i < SHIELD_NUM + 1; ++i)
	{
		memcpy(shields[i].name, n, sizeof(shields[i].name));
		//printf("%s\n", shields[i].name);
		shields[i].tpwr = 0;
		shields[i].mpwr = (4 + i);
		shields[i].itemgraphic = 167;
		shields[i].cost = 1000;
		if (i + 1 < 10)
			n[19] = '0' + (i + 1);
		else
		{
			n[19] = '1';
			n[20] = '0' + ((i + 1) % 10);
		}

	}
}

void ADTA_loadItems( void )
{
	// Unlike the original game, these are loaded once on startup and never again
	ARC_IdentifyPrint("");

	ADTA_loadShots();
	sprintf(tmpBuf.l, "arcshot.dta: load OK (%zu items)", num_pWeapons);
	ARC_IdentifyPrint(tmpBuf.l);

	ADTA_loadPorts();
	sprintf(tmpBuf.l, "arcport.dta: load OK (%zu items)", num_ports);
	ARC_IdentifyPrint(tmpBuf.l);

	ADTA_loadSpecials();
	sprintf(tmpBuf.l, "arcspec.dta: load OK (%zu items)", num_specials);
	ARC_IdentifyPrint(tmpBuf.l);

	ADTA_loadShips();
	sprintf(tmpBuf.l, "arcship.dta: load OK (%zu items)", num_ships);
	ARC_IdentifyPrint(tmpBuf.l);

	ADTA_loadOptions();
	sprintf(tmpBuf.l, "arcopt.dta: load OK (%zu items)", num_options);
	ARC_IdentifyPrint(tmpBuf.l);

	ADTA_loadShields();
}


void JE_loadItemDat( void )
{
	FILE *f = NULL;
	bool isT2000 = false;

	f = dir_fopen_die(data_dir(), levelFile, "rb");
	if (lvlPos[lvlNum-1] == ftell_eof(f))
	{
		// this episode uses the global data files
		fclose(f);

		f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
		efread(&episode1DataLoc, sizeof(JE_longint), 1, f);
		fseek(f, episode1DataLoc, SEEK_SET);
	}
	else
		fseek(f, lvlPos[lvlNum-1], SEEK_SET);

/*
	if (episodeNum <= 3)
	{
		f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
		efread(&episode1DataLoc, sizeof(JE_longint), 1, f);
		fseek(f, episode1DataLoc, SEEK_SET);
	}
	else
	{
		// episode 4 stores item data in the level file
		f = dir_fopen_die(data_dir(), levelFile, "rb");
		fseek(f, lvlPos[lvlNum-1], SEEK_SET);
	}
*/

	JE_word itemNum[7]; /* [1..7] */
	efread(&itemNum, sizeof(JE_word), 7, f);

	// don't use the global tyrian2000detected here, so we can mix and match
	// (mostly useful for one "Camanis 2")
	isT2000 = (itemNum[0] == 818 && itemNum[1] == 60);

	// Only load the first 256 entries (enemy shots)
	// Player shots are loaded with arcade data
	for (int i = 0; i < 256; ++i)
	{
		fseek(f, sizeof(JE_word), SEEK_CUR); // skip ->drain (unused)
		fseek(f, sizeof(JE_byte), SEEK_CUR); // skip ->shotrepeat (unused by enemies)
		efread(&eWeapons[i].multi,           sizeof(JE_byte), 1, f);
		efread(&eWeapons[i].weapani,         sizeof(JE_word), 1, f);
		efread(&eWeapons[i].max,             sizeof(JE_byte), 1, f);
		efread(&eWeapons[i].tx,              sizeof(JE_byte), 1, f);
		efread(&eWeapons[i].ty,              sizeof(JE_byte), 1, f);
		efread(&eWeapons[i].aim,             sizeof(JE_byte), 1, f);
		efread(&eWeapons[i].attack,          sizeof(JE_byte), 8, f);
		efread(&eWeapons[i].del,             sizeof(JE_byte), 8, f);
		efread(&eWeapons[i].sx,              sizeof(JE_shortint), 8, f);
		efread(&eWeapons[i].sy,              sizeof(JE_shortint), 8, f);
		efread(&eWeapons[i].bx,              sizeof(JE_shortint), 8, f);
		efread(&eWeapons[i].by,              sizeof(JE_shortint), 8, f);
		efread(&eWeapons[i].sg,              sizeof(JE_word), 8, f);
		efread(&eWeapons[i].acceleration,    sizeof(JE_shortint), 1, f);
		memset(&eWeapons[i].acceleration,  eWeapons[i].acceleration[0],  sizeof(JE_shortint) * 8); // fill remaining values in
		efread(&eWeapons[i].accelerationx,   sizeof(JE_shortint), 1, f);
		memset(&eWeapons[i].accelerationx, eWeapons[i].accelerationx[0], sizeof(JE_shortint) * 8); // fill remaining values in
		fseek(f, sizeof(JE_byte), SEEK_CUR); // skip ->circlesize (unused by enemies)
		efread(&eWeapons[i].sound,           sizeof(JE_byte), 1, f);
		fseek(f, sizeof(JE_byte), SEEK_CUR); // skip ->trail (unused by enemies)
		fseek(f, sizeof(JE_byte), SEEK_CUR); // skip ->shipblastfilter (unused by enemies)
	}

	// Skip all items between shots and enemies, as we don't use them
	// (we read from arcdata instead)
	fseek(f, (isT2000) ? 0x1ddeb : 0xc7e3, SEEK_CUR);


	const int enemy_num = (isT2000) ? T2KENEMY_NUM : ENEMY_NUM;
	enemyDat = realloc(enemyDat, sizeof(JE_EnemyDatType) * (enemy_num + 1));

	int i = 0, stop = ENEMY_NUM;

	redoForT2KEnemies:
	for (; i < stop + 1; ++i)
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
	}

	if (isT2000 && stop == ENEMY_NUM)
	{
		// Skip 151 entries, start at 1001
		i = T2KENEMY_START;
		stop = T2KENEMY_NUM;
		goto redoForT2KEnemies;
	}

	fclose(f);
}

void JE_initEpisode( JE_byte newEpisode )
{
	if (newEpisode == episodeNum)
		return;
	
	episodeNum = newEpisode;
	
	snprintf(levelFile,    sizeof(levelFile),    "tyrian%d.lvl",  episodeNum);
	snprintf(cube_file,    sizeof(cube_file),    "cubetxt%d.dat", episodeNum);
	snprintf(episode_file, sizeof(episode_file), "levels%d.dat",  episodeNum);
	
	JE_analyzeLevel();
	JE_loadItemDat();
}

void JE_scanForEpisodes( void )
{
	for (int i = 0; i < EPISODE_MAX; ++i)
	{
		char ep_file[20];
		snprintf(ep_file, sizeof(ep_file), "tyrian%d.lvl", i + 1);
		episodeAvail[i] = dir_file_exists(data_dir(), ep_file);
	}
}

unsigned int JE_findNextEpisode( void )
{
	unsigned int newEpisode = episodeNum;
	
	jumpBackToEpisode1 = false;
	
	while (true)
	{
		newEpisode++;
		
		if (newEpisode > EPISODE_MAX)
		{
			newEpisode = 1;
			jumpBackToEpisode1 = true;
			gameHasRepeated = true;
		}
		
		if (episodeAvail[newEpisode-1] || newEpisode == episodeNum)
		{
			break;
		}
	}
	
	return newEpisode;
}

