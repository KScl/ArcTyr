/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  config.c
/// \brief Managing configuration files

#include "arcade.h"
#include "config.h"
#include "file.h"
#include "input.h"
#include "loudness.h"
#include "nortsong.h"
#include "opentyr.h"
#include "player.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"
#include "video/scaler.h"

#include "lib/mtrand.h"

#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#define mkdir _mkdir
#else
#include <unistd.h>
#endif

JE_boolean smoothies[9] = /* [1..9] */
{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };

JE_byte starShowVGASpecialCode;

/* Difficulty */
JE_shortint difficultyLevel;
JE_shortint initialDifficulty; // controls path through levels, etc.

/* Level Data */
char    levelName[11]; /* string [10] */
JE_byte mainLevel, nextLevel;   /*Current Level #*/

/* Configuration */
JE_shortint levelFilter, levelFilterNew, levelBrightness, levelBrightnessChg;
JE_boolean  filtrationAvail, filterActive, filterFade, filterFadeStart;

JE_boolean twoPlayerLinked;

JE_byte    SAPowerupBag[5];
JE_byte    superArcadePowerUp;

JE_real linkGunDirec;

JE_byte background3over;
JE_byte background2over;
JE_byte gammaCorrection;
JE_boolean explosionTransparent,
           youAreCheating,
           background2, smoothScroll, wild, superWild, starActive,
           topEnemyOver,
           skyEnemyOverAll,
           background2notTransparent;

JE_byte    fastPlay;
JE_boolean pentiumMode;

/* Savegame files */
JE_byte    gameSpeed;
JE_byte    processorType;  /* 1=386 2=486 3=Pentium Hyper */

Config tav_config;

const char *get_user_directory( void )
{
	static char user_dir[500] = "";
	
	if (strlen(user_dir) == 0)
	{
#ifndef TARGET_WIN32
		char *xdg_config_home = getenv("XDG_CONFIG_HOME");
		if (xdg_config_home != NULL)
		{
			snprintf(user_dir, sizeof(user_dir), "%s/arctyr", xdg_config_home);
		}
		else
		{
			char *home = getenv("HOME");
			if (home != NULL)
			{
				snprintf(user_dir, sizeof(user_dir), "%s/.config/arctyr", home);
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

void ArcTyr_setGameLevelSettings( void )
{
	static const int startDifficulty[] = {1, 2, 2, 2, 3, 3, 3, 3};
	static const int maxDifficulty[]   = {3, 4, 5, 6, 6, 7, 8, 8};
	static const int rankPerStage[]    = {1, 1, 2, 2, 2, 2, 2, 3};
	static const int rankPerStageAlt[] = {2, 2, 3, 3, 3, 4, 4, 5};
	static const int powerPenalty[]    = {0, 1, 2, 2, 3, 3, 3, 5};
	static const int startPower[]      = {4, 4, 4, 3, 3, 3, 2, 1};
	static const int continuePower[]   = {6, 5, 4, 4, 4, 3, 3, 1};

	if (DIP.gameLevel == 0)
		return;
	int level = DIP.gameLevel - 1;

	DIP.startingDifficulty = startDifficulty[level];
	DIP.difficultyMax = maxDifficulty[level];
	DIP.powerLoss = powerPenalty[level];
	DIP.powerStart = startPower[level];
	DIP.powerContinue = continuePower[level];

	if (DIP.allowMultipleEpisodes)
		DIP.rankUp = rankPerStage[level];
	else
		DIP.rankUp = rankPerStageAlt[level];
}

bool ArcTyr_loadConfig( void )
{
	uint dummy;

	// default openTyrian settings
	fullscreen_enabled = false;
	set_scaler_by_name("2x");

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
	dummy = 0x100;\
	config_get_uint_option(sec, n, &dummy); \
	if (dummy <= max) \
		opt = (JE_byte)(dummy & 0xFF);

#define get_byte_minmax(opt, n, min, max) \
	dummy = 0x100;\
	config_get_uint_option(sec, n, &dummy); \
	if (dummy >= min && dummy <= max) \
		opt = (JE_byte)(dummy & 0xFF);

	if ((sec = config_find_section(&tav_config, "video", NULL)) != NULL)
	{
		get_byte_max(gammaCorrection, "gamma", 4);
		get_byte_max(processorType, "processor", 6);
		JE_initProcessorType();
		JE_setNewGameSpeed();

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
		get_byte_max(DIP.gameLevel, "game_level", 8);
		if (DIP.gameLevel == 0)
		{
			get_byte_minmax(DIP.startingDifficulty, "starting_difficulty", 1, 10);
			get_byte_minmax(DIP.difficultyMax, "difficulty_max", 1, 10);
			get_byte_max(DIP.rankUp, "rank_up", 16);
			get_byte_minmax(DIP.powerStart, "power_start", 1, 11);
			get_byte_minmax(DIP.powerContinue, "power_continue", 1, 11);
			get_byte_max(DIP.powerLoss, "power_loss", 5);
		}
		ArcTyr_setGameLevelSettings();

		get_byte_max(DIP.coinsToStart, "coins_to_start", 8);
		get_byte_max(DIP.coinsToContinue, "coins_to_continue", 8);
		get_byte_minmax(DIP.livesStart, "lives_start", 1, 11);
		get_byte_minmax(DIP.livesContinue, "lives_continue", 1, 11);
			get_byte_max(DIP.rankAffectsScore, "rank_affects_score", 1);

		get_byte_max(DIP.attractSound, "attract_sound", 2);

		get_byte_max(DIP.enableFullDebugMenus, "enable_full_debug_menus", 1);
		get_byte_max(DIP.skipServiceOnStartup, "skip_service_on_startup", 1);
		get_byte_max(DIP.enableMidEpisodeStart, "enable_mid_episode_start", 1);
		get_byte_max(DIP.allowMultipleEpisodes, "allow_multiple_episodes", 1);
	}

#undef get_byte_max
#undef get_byte_minmax

	if (!I_loadConfigAssignments(&tav_config))
		I_resetConfigAssignments();

	return true;
}

bool ArcTyr_saveConfig( void )
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
		config_set_uint_option(sec, "game_level", DIP.gameLevel);
		config_set_uint_option(sec, "starting_difficulty", DIP.startingDifficulty);
		config_set_uint_option(sec, "rank_up", DIP.rankUp);
		config_set_uint_option(sec, "difficulty_max", DIP.difficultyMax);
		config_set_uint_option(sec, "power_start", DIP.powerStart);
		config_set_uint_option(sec, "power_continue", DIP.powerContinue);
		config_set_uint_option(sec, "power_loss", DIP.powerLoss);

		config_set_uint_option(sec, "coins_to_start", DIP.coinsToStart);
		config_set_uint_option(sec, "coins_to_continue", DIP.coinsToContinue);
		config_set_uint_option(sec, "lives_start", DIP.livesStart);
		config_set_uint_option(sec, "lives_continue", DIP.livesContinue);
		config_set_uint_option(sec, "rank_affects_score", DIP.rankAffectsScore);

		config_set_uint_option(sec, "attract_sound", DIP.attractSound);

		config_set_uint_option(sec, "enable_full_debug_menus", DIP.enableFullDebugMenus);
		config_set_uint_option(sec, "skip_service_on_startup", DIP.skipServiceOnStartup);
		config_set_uint_option(sec, "enable_mid_episode_start", DIP.enableMidEpisodeStart);
		config_set_uint_option(sec, "allow_multiple_episodes", DIP.allowMultipleEpisodes);
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

	switch (processorType)
	{
		case 1: /* 386 */
			background2 = false;
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
		case 6: /* Unbounded */
			fastPlay = 6;
			break;
	}

}

void JE_setNewGameSpeed( void )
{
	pentiumMode = false;
	smoothScroll = true;

	switch (fastPlay)
	{
	case 0:
		speed = 0x4300;
		frameCountMax = 2;
		break;
	case 1:
		speed = 0x3000;
		frameCountMax = 2;
		break;
	case 2:
		speed = 0x2000;
		smoothScroll = false;
		frameCountMax = 2;
		break;
	case 3:
		speed = 0x5300;
		frameCountMax = 4;
		break;
	case 4:
		speed = 0x4300;
		frameCountMax = 3;
		break;
	case 5:
		speed = 0x4300;
		frameCountMax = 2;
		pentiumMode = true;
		break;
	case 6:
		speed = 0x4300;
		frameCountMax = 0;
		break;
	}

  frameCount = frameCountMax;
  JE_resetTimerInt();
  JE_setTimerInt();
}
