/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  episodes.c
/// \brief Loading and navigating the game's episodes

#include "arcade.h"
#include "episodes.h"
#include "file.h"
#include "opentyr.h"

#include "mod/patcher.h"

/* Enemy data */
JE_EnemyWeaponType eWeapons[256];
JE_EnemyDatType    enemyDat[T2KENEMY_NUM + 1];

/* EPISODE variables */
JE_byte    initial_episode_num, episodeNum = 0;
JE_boolean episodeAvail[EPISODE_MAX]; /* [1..episodemax] */
char       episode_file[13];
char       level_file[13];

// Location of enemy data inside global (non-episode) files
JE_longint globalDataLoc;

// Level data positions
JE_longint lvlPos[43];
JE_word lvlNum;

// Formerly in lvllib.c, moved here because it relates to loading episodes and nothing else
static void _analyzeLevel( void )
{
	FILE *f = dir_fopen_die(data_dir(), level_file, "rb");
	
	efread(&lvlNum, sizeof(JE_word), 1, f);
	
	for (int x = 0; x < lvlNum; x++)
		efread(&lvlPos[x], sizeof(JE_longint), 1, f);
	
	lvlPos[lvlNum] = ftell_eof(f);
	
	fclose(f);
}

// Loads all data for enemies
// This also used to load player items as well, but that's been separated away
static void _loadEnemyData( void )
{
	FILE *f = dir_fopen_die(data_dir(), level_file, "rb");
	bool isT2000 = false;

	if (lvlPos[lvlNum-1] == ftell_eof(f))
	{
		// this episode uses the global data files
		fclose(f);

		f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
		efread(&globalDataLoc, sizeof(JE_longint), 1, f);
		fseek(f, globalDataLoc, SEEK_SET);
	}
	else
		fseek(f, lvlPos[lvlNum-1], SEEK_SET);

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

void Episode_init( JE_byte newEpisode )
{
	if (newEpisode == episodeNum)
		return;
	
	episodeNum = newEpisode;
	
	snprintf(level_file,   sizeof(level_file),   "tyrian%d.lvl",  episodeNum);
	snprintf(episode_file, sizeof(episode_file), "levels%d.dat",  episodeNum);
	
	_analyzeLevel();
	_loadEnemyData();
	MOD_PatcherSetup(level_file);
}

void Episode_scan( void )
{
	for (int i = 0; i < EPISODE_MAX; ++i)
	{
		char ep_file[20];
		snprintf(ep_file, sizeof(ep_file), "tyrian%d.lvl", i + 1);
		episodeAvail[i] = dir_file_exists(data_dir(), ep_file);
	}
}

unsigned int Episode_getNext( void )
{
	// Single Episode Mode
	if (!DIP.allowMultipleEpisodes)
		return 0;

	unsigned int newEpisode = episodeNum;
	while (++newEpisode <= EPISODE_MAX)
	{
		if (episodeAvail[newEpisode-1])
			return newEpisode;
	}
	return 0;
}

bool Episode_next( void )
{
	unsigned int newEpisode = Episode_getNext();
	if (newEpisode == 0)
		return false;

	if (newEpisode != episodeNum)
		Episode_init(newEpisode);
	return true;
}
