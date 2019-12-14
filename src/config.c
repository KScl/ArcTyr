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
#include "input.h"
#include "loudness.h"
#include "mtrand.h"
#include "nortsong.h"
#include "opentyr.h"
#include "player.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"
#include "video/scaler.h"

#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#define mkdir _mkdir
#else
#include <unistd.h>
#endif

/* Configuration Load/Save handler */

const JE_KeySettingType defaultKeySettings =
{
	SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_SPACE, SDLK_RETURN, SDLK_LCTRL, SDLK_LALT
/*	72, 80, 75, 77, 57, 28, 29, 56*/
};

/* Last 2 bytes = Word
 *
 * Max Value = 1680
 * X div  60 = Armor  (1-28)
 * X div 168 = Shield (1-12)
 * X div 280 = Engine (1-06)
 */


JE_boolean smoothies[9] = /* [1..9] */
{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };

JE_byte starShowVGASpecialCode;

/* CubeData */
JE_word lastCubeMax, cubeMax;
JE_word cubeList[4]; /* [1..4] */

/* Difficulty */
JE_shortint difficultyLevel, oldDifficultyLevel,
            initialDifficulty;  // can only get highscore on initial episode

/* Level Data */
char    lastLevelName[11], levelName[11]; /* string [10] */
JE_byte mainLevel, nextLevel, saveLevel;   /*Current Level #*/

/* Keyboard Junk */
JE_KeySettingType keySettings;

/* Configuration */
JE_shortint levelFilter, levelFilterNew, levelBrightness, levelBrightnessChg;
JE_boolean  filtrationAvail, filterActive, filterFade, filterFadeStart;

JE_boolean gameJustLoaded;

JE_boolean extraGame;

JE_boolean twoPlayerMode, twoPlayerLinked, onePlayerAction, superTyrian;
JE_byte    superArcadeMode;

JE_byte    SAPowerupBag[5];
JE_byte    superArcadePowerUp;

JE_real linkGunDirec;
JE_byte inputDevice[2] = { 1, 2 }; // 0:any  1:keyboard  2:mouse  3+:joystick

JE_byte secretHint;
JE_byte background3over;
JE_byte background2over;
JE_byte gammaCorrection;
JE_boolean explosionTransparent,
           youAreCheating,
           displayScore,
           background2, smoothScroll, wild, superWild, starActive,
           topEnemyOver,
           skyEnemyOverAll,
           background2notTransparent;

JE_byte    fastPlay;
JE_boolean pentiumMode;

/* Savegame files */
JE_byte    gameSpeed;
JE_byte    processorType;  /* 1=386 2=486 3=Pentium Hyper */

JE_SaveFilesType saveFiles; /*array[1..saveLevelnum] of savefiletype;*/
JE_SaveGameTemp saveTemp;

Config opentyrian_config;  // implicitly initialized
Config tav_config;

static bool _TAV_loadConfig( void )
{
	uint dummy;

	// default openTyrian settings
	fullscreen_enabled = false;
	set_scaler_by_name("2x");

	memcpy(&keySettings, &defaultKeySettings, sizeof(keySettings));

	// tyrian.cfg video settings
	gammaCorrection = 0;
	processorType = 4;
	gameSpeed = 4;
	JE_initProcessorType();

	// tyrian.cfg audio settings
	tyrMusicVolume = fxVolume = 128;
	JE_calcFXVol();
	set_volume(tyrMusicVolume, fxVolume);

	// set default DIP switch values
	memcpy(&DIP, &DIP_Default, sizeof(DIP));

	// set assignments
	I_resetConfigAssignments();

	FILE *file = dir_fopen_warn(get_user_directory(), "tav.cfg", "r");
	if (file == NULL)
		return false;

	if (!config_parse(&tav_config, file))
	{
		fclose(file);
		return false;
	}

	ConfigSection *sec;

#define get_byte_max(opt, n, max) \
	config_get_uint_option(sec, n, &dummy); \
	if (dummy <= max) \
		opt = (JE_byte)(dummy & 0xFF);

#define get_byte_minmax(opt, n, min, max) \
	config_get_uint_option(sec, n, &dummy); \
	if (dummy >= min && dummy <= max) \
		opt = (JE_byte)(dummy & 0xFF);

	if ((sec = config_find_section(&tav_config, "video", NULL)) != NULL)
	{
		get_byte_max(gammaCorrection, "gamma", 4);
		get_byte_max(processorType, "processor", 6);
		get_byte_max(gameSpeed, "speed", 5);
		JE_initProcessorType();

		config_get_bool_option(sec, "fullscreen", &fullscreen_enabled);
		
		const char *scaler;
		if (config_get_string_option(sec, "scaler", &scaler))
			set_scaler_by_name(scaler);
	}

	if ((sec = config_find_section(&tav_config, "audio", NULL)) != NULL)
	{
		config_get_uint_option(sec, "music_volume", &dummy);
		tyrMusicVolume = (JE_word)(dummy & 0xFFFF);
		config_get_uint_option(sec, "sfx_volume", &dummy);
		fxVolume = (JE_word)(dummy & 0xFFFF);

		if (tyrMusicVolume > 255)
			tyrMusicVolume = 255;
		if (fxVolume > 255)
			fxVolume = 255;

		JE_calcFXVol();
		set_volume(tyrMusicVolume, fxVolume);
	}

	if ((sec = config_find_section(&tav_config, "dip_switches", NULL)) != NULL)
	{
		get_byte_minmax(DIP.startingDifficulty, "starting_difficulty", 1, 10);
		get_byte_max(DIP.rankUp, "rank_up", 16);
		get_byte_minmax(DIP.difficultyMax, "difficulty_max", 1, 10);
		get_byte_max(DIP.rankAffectsScore, "rank_affects_score", 1);

		get_byte_max(DIP.coinsPerGame, "coins_per_game", 8);
		get_byte_minmax(DIP.livesStart, "lives_start", 1, 11);
		get_byte_minmax(DIP.livesContinue, "lives_continue", 1, 11);
		get_byte_minmax(DIP.powerStart, "power_start", 1, 11);
		get_byte_minmax(DIP.powerContinue, "power_continue", 1, 11);

		get_byte_max(DIP.attractSound, "attract_sound", 2);
	}

#undef get_byte_max
#undef get_byte_minmax

	if (!I_loadConfigAssignments(&tav_config))
		I_resetConfigAssignments();

	return true;
}

static bool _TAV_saveConfig( void )
{
	ConfigSection *sec;

	if ((sec = config_find_or_add_section(&tav_config, "video", NULL)) != NULL)
	{
		config_set_uint_option(sec, "gamma", gammaCorrection);
		config_set_uint_option(sec, "processor", processorType);
		config_set_uint_option(sec, "speed", gameSpeed);

		config_set_bool_option(sec, "fullscreen", fullscreen_enabled, NO_YES);
		config_set_string_option(sec, "scaler", scalers[scaler].name);
	}

	if ((sec = config_find_or_add_section(&tav_config, "audio", NULL)) != NULL)
	{
		config_set_uint_option(sec, "music_volume", tyrMusicVolume);
		config_set_uint_option(sec, "sfx_volume", fxVolume);
	}

	if ((sec = config_find_or_add_section(&tav_config, "dip_switches", NULL)) != NULL)
	{
		config_set_uint_option(sec, "starting_difficulty", DIP.startingDifficulty);
		config_set_uint_option(sec, "rank_up", DIP.rankUp);
		config_set_uint_option(sec, "difficulty_max", DIP.difficultyMax);
		config_set_uint_option(sec, "rank_affects_score", DIP.rankAffectsScore);

		config_set_uint_option(sec, "coins_per_game", DIP.coinsPerGame);
		config_set_uint_option(sec, "lives_start", DIP.livesStart);
		config_set_uint_option(sec, "lives_continue", DIP.livesContinue);
		config_set_uint_option(sec, "power_start", DIP.powerStart);
		config_set_uint_option(sec, "power_continue", DIP.powerContinue);

		config_set_uint_option(sec, "attract_sound", DIP.attractSound);
	}

	I_saveConfigAssignments(&tav_config);
	
#ifndef TARGET_WIN32
	mkdir(get_user_directory(), 0700);
#else
	mkdir(get_user_directory());
#endif
	
	FILE *file = dir_fopen(get_user_directory(), "tav.cfg", "w");
	if (file == NULL)
		return false;

	config_write(&tav_config, file);

#ifndef TARGET_WIN32
	fsync(fileno(file));
#endif
	fclose(file);

	return true;
}

void JE_initProcessorType( void )
{
	/* SYN: Originally this proc looked at your hardware specs and chose appropriate options. We don't care, so I'll just set
	   decent defaults here. */

	wild = false;
	superWild = false;
	smoothScroll = true;
	explosionTransparent = true;
	filtrationAvail = false;
	background2 = true;
	displayScore = true;

	switch (processorType)
	{
		case 1: /* 386 */
			background2 = false;
			displayScore = false;
			explosionTransparent = false;
			break;
		case 2: /* 486 - Default */
			break;
		case 3: /* High Detail */
			smoothScroll = false;
			break;
		case 4: /* Pentium */
			wild = true;
			filtrationAvail = true;
			break;
		case 5: /* Nonstandard VGA */
			smoothScroll = false;
			break;
		case 6: /* SuperWild */
			wild = true;
			superWild = true;
			filtrationAvail = true;
			break;
	}

	switch (gameSpeed)
	{
		case 1:  /* Slug Mode */
			fastPlay = 3;
			break;
		case 2:  /* Slower */
			fastPlay = 4;
			break;
		case 3: /* Slow */
			fastPlay = 5;
			break;
		case 4: /* Normal */
			fastPlay = 0;
			break;
		case 5: /* Pentium Hyper */
			fastPlay = 1;
			break;
	}

}

void JE_setNewGameSpeed( void )
{
	pentiumMode = false;

	switch (fastPlay)
	{
	case 0:
		speed = 0x4300;
		smoothScroll = true;
		frameCountMax = 2;
		break;
	case 1:
		speed = 0x3000;
		smoothScroll = true;
		frameCountMax = 2;
		break;
	case 2:
		speed = 0x2000;
		smoothScroll = false;
		frameCountMax = 2;
		break;
	case 3:
		speed = 0x5300;
		smoothScroll = true;
		frameCountMax = 4;
		break;
	case 4:
		speed = 0x4300;
		smoothScroll = true;
		frameCountMax = 3;
		break;
	case 5:
		speed = 0x4300;
		smoothScroll = true;
		frameCountMax = 2;
		pentiumMode = true;
		break;
	}

  frameCount = frameCountMax;
  JE_resetTimerInt();
  JE_setTimerInt();
}

void JE_encryptSaveTemp( void )
{
	STUB();
/*
	JE_SaveGameTemp s3;
	JE_word x;
	JE_byte y;

	memcpy(&s3, &saveTemp, sizeof(s3));

	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y += s3[x];
	}
	saveTemp[SAVE_FILE_SIZE] = y;

	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y -= s3[x];
	}
	saveTemp[SAVE_FILE_SIZE+1] = y;

	y = 1;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y = (y * s3[x]) + 1;
	}
	saveTemp[SAVE_FILE_SIZE+2] = y;

	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y = y ^ s3[x];
	}
	saveTemp[SAVE_FILE_SIZE+3] = y;

	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		saveTemp[x] = saveTemp[x] ^ cryptKey[(x+1) % 10];
		if (x > 0)
		{
			saveTemp[x] = saveTemp[x] ^ saveTemp[x - 1];
		}
	}
*/
}

void JE_decryptSaveTemp( void )
{
	STUB();
/*
	JE_boolean correct = true;
	JE_SaveGameTemp s2;
	int x;
	JE_byte y;

	// Decrypt save game file
	for (x = (SAVE_FILE_SIZE - 1); x >= 0; x--)
	{
		s2[x] = (JE_byte)saveTemp[x] ^ (JE_byte)(cryptKey[(x+1) % 10]);
		if (x > 0)
		{
			s2[x] ^= (JE_byte)saveTemp[x - 1];
		}

	}

	for (x = 0; x < SAVE_FILE_SIZE; x++) printf("%c", s2[x]);

	// Check save file for correctitude
	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y += s2[x];
	}
	if (saveTemp[SAVE_FILE_SIZE] != y)
	{
		correct = false;
		printf("Failed additive checksum: %d vs %d\n", saveTemp[SAVE_FILE_SIZE], y);
	}

	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y -= s2[x];
	}
	if (saveTemp[SAVE_FILE_SIZE+1] != y)
	{
		correct = false;
		printf("Failed subtractive checksum: %d vs %d\n", saveTemp[SAVE_FILE_SIZE+1], y);
	}

	y = 1;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y = (y * s2[x]) + 1;
	}
	if (saveTemp[SAVE_FILE_SIZE+2] != y)
	{
		correct = false;
		printf("Failed multiplicative checksum: %d vs %d\n", saveTemp[SAVE_FILE_SIZE+2], y);
	}

	y = 0;
	for (x = 0; x < SAVE_FILE_SIZE; x++)
	{
		y = y ^ s2[x];
	}
	if (saveTemp[SAVE_FILE_SIZE+3] != y)
	{
		correct = false;
		printf("Failed XOR'd checksum: %d vs %d\n", saveTemp[SAVE_FILE_SIZE+3], y);
	}

	// Barf and die if save file doesn't validate
	if (!correct)
	{
		fprintf(stderr, "Error reading save file!\n");
		exit(255);
	}

	// Keep decrypted version plz
	memcpy(&saveTemp, &s2, sizeof(s2));
*/
}

const char *get_user_directory( void )
{
	static char user_dir[500] = "";
	
	if (strlen(user_dir) == 0)
	{
#ifndef TARGET_WIN32
		char *xdg_config_home = getenv("XDG_CONFIG_HOME");
		if (xdg_config_home != NULL)
		{
			snprintf(user_dir, sizeof(user_dir), "%s/opentyrian", xdg_config_home);
		}
		else
		{
			char *home = getenv("HOME");
			if (home != NULL)
			{
				snprintf(user_dir, sizeof(user_dir), "%s/.config/opentyrian-mod", home);
			}
			else
			{
				strcpy(user_dir, ".");
			}
		}
#else
		strcpy(user_dir, ".");
#endif
	}
	
	return user_dir;
}

void JE_loadConfiguration( void )
{
	//int z;
	_TAV_loadConfig();
/*
	FILE *fi;
	JE_byte *p;
	int y;
	
	fi = dir_fopen_warn(get_user_directory(), "tyrian.cfg", "rb");
	if (fi && ftell_eof(fi) == 20 + sizeof(keySettings))
	{
		// SYN: I've hardcoded the sizes here because the .CFG file format is fixed
		// anyways, so it's not like they'll change.
		background2 = 0;
		efread(&background2, 1, 1, fi);
		efread(&gameSpeed, 1, 1, fi);
		
		efread(&inputDevice_, 1, 1, fi);
		efread(&jConfigure, 1, 1, fi);
		
		efread(&versionNum, 1, 1, fi);
		
		efread(&processorType, 1, 1, fi);
		efread(&midiPort, 1, 1, fi);
		efread(&soundEffects, 1, 1, fi);
		efread(&gammaCorrection, 1, 1, fi);
		efread(&difficultyLevel, 1, 1, fi);
		
		efread(joyButtonAssign, 1, 4, fi);
		
		efread(&tyrMusicVolume, 2, 1, fi);
		efread(&fxVolume, 2, 1, fi);
		
		efread(inputDevice, 1, 2, fi);
		
		efread(keySettings, sizeof(*keySettings), COUNTOF(keySettings), fi);
		
		fclose(fi);
	}
	else
	{
		printf("\nInvalid or missing TYRIAN.CFG! Continuing using defaults.\n\n");
		
		soundEffects = 1;
		memcpy(&keySettings, &defaultKeySettings, sizeof(keySettings));
		background2 = true;
		tyrMusicVolume = fxVolume = 128;
		gammaCorrection = 0;
		processorType = 3;
		gameSpeed = 4;
	}
	
	load_opentyrian_config();
	
	if (tyrMusicVolume > 255)
		tyrMusicVolume = 255;
	if (fxVolume > 255)
		fxVolume = 255;
	
	JE_calcFXVol();
	
	set_volume(tyrMusicVolume, fxVolume);

	(void)p;
*/
/*
	fi = dir_fopen_warn(get_user_directory(), "tyrian.sav", "rb");
	if (fi)
	{

		fseek(fi, 0, SEEK_SET);
		efread(saveTemp, 1, sizeof(saveTemp), fi);
		JE_decryptSaveTemp();

		// SYN: The original mostly blasted the save file into raw memory. However, our lives are not so
		// easy, because the C struct is necessarily a different size. So instead we have to loop
		// through each record and load fields manually. *emo tear* :'( 

		p = saveTemp;
		for (z = 0; z < SAVE_FILES_NUM; z++)
		{
			memcpy(&saveFiles[z].encode, p, sizeof(JE_word)); p += 2;
			saveFiles[z].encode = SDL_SwapLE16(saveFiles[z].encode);
			
			memcpy(&saveFiles[z].level, p, sizeof(JE_word)); p += 2;
			saveFiles[z].level = SDL_SwapLE16(saveFiles[z].level);
			
			memcpy(&saveFiles[z].items, p, sizeof(JE_PItemsType)); p += sizeof(JE_PItemsType);
			
			memcpy(&saveFiles[z].score, p, sizeof(JE_longint)); p += 4;
			saveFiles[z].score = SDL_SwapLE32(saveFiles[z].score);
			
			memcpy(&saveFiles[z].score2, p, sizeof(JE_longint)); p += 4;
			saveFiles[z].score2 = SDL_SwapLE32(saveFiles[z].score2);
			
			// SYN: Pascal strings are prefixed by a byte holding the length!
			memset(&saveFiles[z].levelName, 0, sizeof(saveFiles[z].levelName));
			memcpy(&saveFiles[z].levelName, &p[1], *p);
			p += 10;
			
			// This was a BYTE array, not a STRING, in the original. Go fig.
			memcpy(&saveFiles[z].name, p, 14);
			p += 14;
			
			memcpy(&saveFiles[z].cubes, p, sizeof(JE_byte)); p++;
			memcpy(&saveFiles[z].power, p, sizeof(JE_byte) * 2); p += 2;
			memcpy(&saveFiles[z].episode, p, sizeof(JE_byte)); p++;
			memcpy(&saveFiles[z].lastItems, p, sizeof(JE_PItemsType)); p += sizeof(JE_PItemsType);
			memcpy(&saveFiles[z].difficulty, p, sizeof(JE_byte)); p++;
			memcpy(&saveFiles[z].secretHint, p, sizeof(JE_byte)); p++;
			memcpy(&saveFiles[z].input1, p, sizeof(JE_byte)); p++;
			memcpy(&saveFiles[z].input2, p, sizeof(JE_byte)); p++;
			
			// booleans were 1 byte in pascal -- working around it
			Uint8 temp;
			memcpy(&temp, p, 1); p++;
			saveFiles[z].gameHasRepeated = temp != 0;
			
			memcpy(&saveFiles[z].initialDifficulty, p, sizeof(JE_byte)); p++;
			
			memcpy(&saveFiles[z].highScore1, p, sizeof(JE_longint)); p += 4;
			saveFiles[z].highScore1 = SDL_SwapLE32(saveFiles[z].highScore1);
			
			memcpy(&saveFiles[z].highScore2, p, sizeof(JE_longint)); p += 4;
			saveFiles[z].highScore2 = SDL_SwapLE32(saveFiles[z].highScore2);
			
			memset(&saveFiles[z].highScoreName, 0, sizeof(saveFiles[z].highScoreName));
			memcpy(&saveFiles[z].highScoreName, &p[1], *p);
			p += 30;
			
			memcpy(&saveFiles[z].highScoreDiff, p, sizeof(JE_byte)); p++;
		}

		// SYN: This is truncating to bytes. I have no idea what this is doing or why.
		// TODO: Figure out what this is about and make sure it isn't broked.
		editorLevel = (saveTemp[SIZEOF_SAVEGAMETEMP - 5] << 8) | saveTemp[SIZEOF_SAVEGAMETEMP - 6];

		fclose(fi);
	}
	else */ 
/*
	{
		// We didn't have a save file! Let's make up random stuff!
		editorLevel = 800;

		for (z = 0; z < 100; z++)
		{
			saveTemp[SAVE_FILES_SIZE + z] = initialItemAvail[z];
		}

		for (z = 0; z < SAVE_FILES_NUM; z++)
		{
			saveFiles[z].level = 0;

			for (y = 0; y < 14; y++)
			{
				saveFiles[z].name[y] = ' ';
			}
			saveFiles[z].name[14] = 0;

			saveFiles[z].highScore1 = ((mt_rand() % 20) + 1) * 1000;

			if (z % 6 > 2)
			{
				saveFiles[z].highScore2 = ((mt_rand() % 20) + 1) * 1000;
				strcpy(saveFiles[z].highScoreName, defaultTeamNames[mt_rand() % 22]);
			} else {
				strcpy(saveFiles[z].highScoreName, defaultHighScoreNames[mt_rand() % 34]);
			}
		}
	}
	*/
	//JE_initProcessorType();
}

void JE_saveConfiguration( void )
{
	_TAV_saveConfig();
/*
	FILE *f;
	JE_byte *p;
	int z;

	(void)p;
	(void)z;
*/
/*
	p = saveTemp;
	for (z = 0; z < SAVE_FILES_NUM; z++)
	{
		JE_SaveFileType tempSaveFile;
		memcpy(&tempSaveFile, &saveFiles[z], sizeof(tempSaveFile));
		
		tempSaveFile.encode = SDL_SwapLE16(tempSaveFile.encode);
		memcpy(p, &tempSaveFile.encode, sizeof(JE_word)); p += 2;
		
		tempSaveFile.level = SDL_SwapLE16(tempSaveFile.level);
		memcpy(p, &tempSaveFile.level, sizeof(JE_word)); p += 2;
		
		memcpy(p, &tempSaveFile.items, sizeof(JE_PItemsType)); p += sizeof(JE_PItemsType);
		
		tempSaveFile.score = SDL_SwapLE32(tempSaveFile.score);
		memcpy(p, &tempSaveFile.score, sizeof(JE_longint)); p += 4;
		
		tempSaveFile.score2 = SDL_SwapLE32(tempSaveFile.score2);
		memcpy(p, &tempSaveFile.score2, sizeof(JE_longint)); p += 4;
		
		// SYN: Pascal strings are prefixed by a byte holding the length!
		memset(p, 0, sizeof(tempSaveFile.levelName));
		*p = strlen(tempSaveFile.levelName);
		memcpy(&p[1], &tempSaveFile.levelName, *p);
		p += 10;
		
		// This was a BYTE array, not a STRING, in the original. Go fig.
		memcpy(p, &tempSaveFile.name, 14);
		p += 14;
		
		memcpy(p, &tempSaveFile.cubes, sizeof(JE_byte)); p++;
		memcpy(p, &tempSaveFile.power, sizeof(JE_byte) * 2); p += 2;
		memcpy(p, &tempSaveFile.episode, sizeof(JE_byte)); p++;
		memcpy(p, &tempSaveFile.lastItems, sizeof(JE_PItemsType)); p += sizeof(JE_PItemsType);
		memcpy(p, &tempSaveFile.difficulty, sizeof(JE_byte)); p++;
		memcpy(p, &tempSaveFile.secretHint, sizeof(JE_byte)); p++;
		memcpy(p, &tempSaveFile.input1, sizeof(JE_byte)); p++;
		memcpy(p, &tempSaveFile.input2, sizeof(JE_byte)); p++;
		
		// booleans were 1 byte in pascal -- working around it
		Uint8 temp = tempSaveFile.gameHasRepeated != false;
		memcpy(p, &temp, 1); p++;
		
		memcpy(p, &tempSaveFile.initialDifficulty, sizeof(JE_byte)); p++;
		
		tempSaveFile.highScore1 = SDL_SwapLE32(tempSaveFile.highScore1);
		memcpy(p, &tempSaveFile.highScore1, sizeof(JE_longint)); p += 4;
		
		tempSaveFile.highScore2 = SDL_SwapLE32(tempSaveFile.highScore2);
		memcpy(p, &tempSaveFile.highScore2, sizeof(JE_longint)); p += 4;
		
		memset(p, 0, sizeof(tempSaveFile.highScoreName));
		*p = strlen(tempSaveFile.highScoreName);
		memcpy(&p[1], &tempSaveFile.highScoreName, *p);
		p += 30;
		
		memcpy(p, &tempSaveFile.highScoreDiff, sizeof(JE_byte)); p++;
	}
	
	saveTemp[SIZEOF_SAVEGAMETEMP - 6] = editorLevel >> 8;
	saveTemp[SIZEOF_SAVEGAMETEMP - 5] = editorLevel;
	
	JE_encryptSaveTemp();
	*/
/*	
	f = dir_fopen_warn(get_user_directory(), "tyrian.sav", "wb");
	if (f != NULL)
	{
		efwrite(saveTemp, 1, sizeof(saveTemp), f);

#ifndef TARGET_WIN32
		fsync(fileno(f));
#endif
		fclose(f);
	}
	
	JE_decryptSaveTemp();
*/
/*
	f = dir_fopen_warn(get_user_directory(), "tyrian.cfg", "wb");
	if (f != NULL)
	{
		efwrite(&background2, 1, 1, f);
		efwrite(&gameSpeed, 1, 1, f);
		
		efwrite(&inputDevice_, 1, 1, f);
		efwrite(&jConfigure, 1, 1, f);
		
		efwrite(&versionNum, 1, 1, f);
		efwrite(&processorType, 1, 1, f);
		efwrite(&midiPort, 1, 1, f);
		efwrite(&soundEffects, 1, 1, f);
		efwrite(&gammaCorrection, 1, 1, f);
		efwrite(&difficultyLevel, 1, 1, f);
		efwrite(joyButtonAssign, 1, 4, f);
		
		efwrite(&tyrMusicVolume, 2, 1, f);
		efwrite(&fxVolume, 2, 1, f);
		
		efwrite(inputDevice, 1, 2, f);
		
		efwrite(keySettings, sizeof(*keySettings), COUNTOF(keySettings), f);
		
#ifndef TARGET_WIN32
		fsync(fileno(f));
#endif
		fclose(f);
	}
	
	save_opentyrian_config();
*/
}

