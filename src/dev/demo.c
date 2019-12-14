/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  devextra.c
/// \brief Extra menus and functions to aide in development

#include "../arcade.h"
#include "../arcserv.h"
#include "../backgrnd.h"
#include "../config.h"
#include "../episodes.h"
#include "../file.h"
#include "../fonthand.h"
#include "../helptext.h"
#include "../input.h"
#include "../loudness.h"
#include "../mainint.h"
#include "../nortsong.h"
#include "../opentyr.h"
#include "../palette.h"
#include "../picload.h"
#include "../player.h"
#include "../shots.h"
#include "../sndmast.h"
#include "../video.h"
#include "../vga256d.h"

#include "../mtrand.h"

#include <time.h>

struct {
	const char *name;
	JE_byte ep, lev;
} official_levels[] = {
	{"Tyrian",    1, 3},
	{"Asteroid1", 1, 5},
	{"Asteroid2", 1, 6},
	{"Savara",    1, 10},
	{"Mines",     1, 13},
	{"*Bubbles",  1, 16},
	{"Deliani",   1, 19},
	{"*Asteroid?",1, 21},
	{"*MineMaze", 1, 23},
	{"Bonus",     1, 25},
	{"*Holes",    1, 28},
	{"*Soh Jin",  1, 31},
	{"*Windy",    1, 33},
	{"Assassin",  1, 36},
	{"Savara V",  1, 38},
	//{"$ALE",      1, 41},

	{"Torm",      2, 2},
	{"Gyges",     2, 4},
	{"Bonus 1",   2, 6},
	{"Ast. City", 2, 8},
	{"Soh Jin",   2, 10},
	{"Gryphon",   2, 12},
	{"*Gem War",  2, 15},
	{"*Markers",  2, 17},
	{"*Mistakes", 2, 19},
	{"Botany A",  2, 21},
	{"Botany B",  2, 22},
	{"Bonus 2",   2, 24},

	{"Gauntlet",  3, 2},
	{"Ixmucane",  3, 4},
	{"Bonus",     3, 6},
	{"Stargate",  3, 8},
	{"Camanis",   3, 10},
	{"Fleet",     3, 12},
	{"*Ast. City",3, 15},
	{"Tyrian X",  3, 17},
	{"*Sawblades",3, 19},
	{"Savara Y",  3, 21},
	{"New Deli",  3, 23},
	{"Maces",     3, 25},

	{"Surface",   4, 2},
	{"Lava Run",  4, 4},
	{"Core",      4, 6},
	{"?Tunnel?",  4, 8},
	{"Ice Exit",  4, 10},
	{"Harvest",   4, 12},
	{"Underdeli", 4, 14},
	{"Savara IV", 4, 16},
	{"Dread-Not", 4, 18},
	{"Eyespy",    4, 20},
	{"Brainiac",  4, 22},
	{"Nose Drip", 4, 25},
	{"*Windy",    4, 27},
	{"*DesertRun",4, 29},
	{"Lava Exit", 4, 31},
	{"Side Exit", 4, 36},
	//{"$Time War", 4, 40},
	//{"$Squadron", 4, 45},
	{"Approach",  4, 47},
	{"*IceSecret",4, 50},

	// T2000
	{"Coral",     5, 14},
};

Uint8 new_demo_num = 0;

// Options
static JE_byte opt_ship[2] = {255, 254};
static JE_byte opt_shield[2] = {255, 255};
static JE_byte opt_lives[2] = {3, 3};
static JE_byte opt_weapon[2] = {255, 255};
static JE_byte opt_power[2] = {255, 255};
static JE_byte opt_level = 255;
static JE_byte opt_difficulty = 3;
static JE_word opt_timer = 60;

//
//
//
void DEV_RecordDemoInit( void )
{
	int p;
	bool ourFadeIn = true;
	bool ready = false;
	uint button;
	Uint64 new_time = 9999999, enter_time = SDL_GetTicks();

	JE_byte onOption[2] = {0, 0};

	// Technically...
	inServiceMenu = true;
	skip_header_draw = true;

	fade_black(10);

	play_song(DEFAULT_SONG_BUY);

	JE_loadPic(VGAScreen, 2, true);

	JE_dString(VGAScreen, JE_fontCenter("Record Demo", FONT_SHAPES), 12, "Record Demo", FONT_SHAPES);

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	while (!ready)
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen2->pitch * VGAScreen2->h);

		for (p = 0; p < 2; ++p)
		{
			uint shipNum = shiporder[opt_ship[p]];

			if (opt_ship[p] == 254)
				strcpy(tmpBuf.l, "No Player");
			else if (opt_ship[p] == 255)
				strcpy(tmpBuf.l, "Random");
			else
				sprintf(tmpBuf.l, "%s", JE_trim(ships[shipNum].name));
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT) - 60 + 120 * p, 60, tmpBuf.l, 15, (onOption[p] == 0 ? 4 : 0), FULL_SHADE);

			if (opt_ship[p] == 254)
				continue;

			if (opt_shield[p] == 255)
				strcpy(tmpBuf.l, "Shield Level Random");
			else
				sprintf(tmpBuf.l, "Shield Level %d", opt_shield[p] + 1);
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT) - 60 + 120 * p, 70, tmpBuf.l, 15, (onOption[p] == 1 ? 4 : 0), FULL_SHADE);

			sprintf(tmpBuf.l, "%d Lives", opt_lives[p]);
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT) - 60 + 120 * p, 80, tmpBuf.l, 15, (onOption[p] == 2 ? 4 : 0), FULL_SHADE);

			if (opt_weapon[p] == 255)
				strcpy(tmpBuf.l, "Random Weapon");
			else if (opt_ship[p] < 128)
				sprintf(tmpBuf.l, "%s", JE_trim(weaponPort[ships[shipNum].port_weapons[opt_weapon[p]]].name));
			else
				sprintf(tmpBuf.l, "Weapon %d", opt_weapon[p] + 1);
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT) - 60 + 120 * p, 90, tmpBuf.l, 15, (onOption[p] == 3 ? 4 : 0), FULL_SHADE);

			if (opt_power[p] == 255)
				strcpy(tmpBuf.l, "Power Random");
			else
				sprintf(tmpBuf.l, "Power %d", opt_power[p] + 1);
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT) - 60 + 120 * p, 100, tmpBuf.l, 15, (onOption[p] == 4 ? 4 : 0), FULL_SHADE);

		}

		if (opt_level == 255)
			strcpy(tmpBuf.l, "Random Level");
		else
			sprintf(tmpBuf.l, "Episode %d: %s", official_levels[opt_level].ep, official_levels[opt_level].name);
		JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 115, tmpBuf.l, 15, (onOption[0] == 5 ? 4 : 0), FULL_SHADE);

		sprintf(tmpBuf.l, "%s", difficultyNameB[opt_difficulty + 1]);
		JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 125, tmpBuf.l, 15, (onOption[0] == 6 ? 4 : 0), FULL_SHADE);

		sprintf(tmpBuf.l, "%02d:%02d", opt_timer / 60, opt_timer % 60);
		JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 135, tmpBuf.l, 15, (onOption[0] == 7 ? 4 : 0), FULL_SHADE);

		if (new_demo_num != 0)
		{
			new_time = SDL_GetTicks() - enter_time;
			if (new_time < 2000)
			{
				snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%s/arcdemo.%d", get_user_directory(), new_demo_num);
				int brightness = (new_time % 500 < 250) ? (new_time % 250 / 50) : 5 - ((new_time % 250) / 50);
				JE_textShade(VGAScreen, JE_fontCenter("Demo saved as", TINY_FONT), 30, "Demo saved as", 15, brightness, FULL_SHADE);
				JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 39, tmpBuf.l, 15, brightness, FULL_SHADE);
			}			
		}

		if (ourFadeIn)
		{
			fade_palette(colors, 10, 0, 255);
			ourFadeIn = false;
		}

		JE_showVGA();
		wait_delay();

		button = INPUT_P1_UP;
		if (new_time < 2000)
			I_checkButtons();
		else
			I_waitOnInputForMenu(button, INPUT_SERVICE_ENTER, 0);
		while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			p = 0;
			switch (button++)
			{
			case INPUT_P2_UP:
				if (opt_ship[1] == 254)
					break;

				p = 1;
				// fall through
			case INPUT_P1_UP:
				JE_playSampleNum(S_CURSOR);
				if (--onOption[p] == 255)
					onOption[p] = (p == 1) ? 4 : 7;
				break;

			case INPUT_P2_DOWN:
				if (opt_ship[1] == 254)
					break;

				p = 1;
				// fall through
				case INPUT_P1_DOWN:
				JE_playSampleNum(S_CURSOR);
				if ((p == 0 && ++onOption[p] > 7)
				 || (p == 1 && ++onOption[p] > 4))
					onOption[p] = 0;
				break;

			case INPUT_P2_LEFT:
				p = 1;
				// fall through
			case INPUT_P1_LEFT:
				switch(onOption[p])
				{
				case 0:
					if (--opt_ship[p] == 253)
						opt_ship[p] = SHIPORDER_MAX - 1;
					break;
				case 1:
					if (--opt_shield[p] == 254)
						opt_shield[p] = 9;
					break;
				case 2:
					if (--opt_lives[p] == 0)
						opt_lives[p] = 11;
					break;
				case 3:
					if (--opt_weapon[p] == 254)
						opt_weapon[p] = 4;
					break;
				case 4:
					if (--opt_power[p] == 254)
						opt_power[p] = 10;
					break;
				case 5:
					do 
					{
						if (--opt_level == 254)
							opt_level = COUNTOF(official_levels) - 1;
					} while (opt_level != 255 && !episodeAvail[official_levels[opt_level].ep - 1]);
					break;
				case 6:
					if (--opt_difficulty == 0)
						opt_difficulty = 9;
					break;
				case 7:
					if ((opt_timer -= 5) < 15)
						opt_timer = 120;
					break;
				}
				JE_playSampleNum(S_CURSOR);
				break;

			case INPUT_P2_RIGHT:
				p = 1;
				// fall through
			case INPUT_P1_RIGHT:
				switch(onOption[p])
				{
				case 0:
					if (++opt_ship[p] == SHIPORDER_MAX)
						opt_ship[p] = 254;
					break;
				case 1:
					if (++opt_shield[p] == 10)
						opt_shield[p] = 255;
					break;
				case 2:
					if (++opt_lives[p] == 12)
						opt_lives[p] = 1;
					break;
				case 3:
					if (++opt_weapon[p] == 5)
						opt_weapon[p] = 255;
					break;
				case 4:
					if (++opt_power[p] == 11)
						opt_power[p] = 255;
					break;
				case 5:
					do
					{
						if (++opt_level == COUNTOF(official_levels))
							opt_level = 255;
					}
					while (opt_level != 255 && !episodeAvail[official_levels[opt_level].ep - 1]);
					break;
				case 6:
					if (++opt_difficulty == 10)
						opt_difficulty = 1;
					break;
				case 7:
					if ((opt_timer += 5) > 120)
						opt_timer = 15;
					break;
				}
				JE_playSampleNum(S_CURSOR);
				break;

			case INPUT_P1_FIRE:
			case INPUT_P2_FIRE:
				if (opt_ship[0] == opt_ship[1] && opt_ship[0] != 255)
				{
					JE_playSampleNum(S_CLINK);
					break;
				}
				JE_playSampleNum(S_SELECT);
				ready = true;
				break;

			case INPUT_P2_MODE:
				p = 1;
				// fall through
			case INPUT_P1_MODE:
				switch(onOption[p])
				{
				case 0: opt_ship[p] = (p == 0 ? 255 : 254); break;
				case 1: opt_shield[p] = 255; break;
				case 2: opt_lives[p] = 3; break;
				case 3: opt_weapon[p] = 255; break;
				case 4: opt_power[p] = 255; break;
				case 5: opt_level = 255; break;
				case 6: opt_difficulty = 3; break;
				case 7: opt_timer = 60; break;
				}
				JE_playSampleNum(S_SPRING);
				break;

			case INPUT_SERVICE_ENTER:
				JE_playSampleNum(S_SPRING);
				fade_black(10);
				JE_tyrianHalt(0);
			}
		}
	}
	fade_black(10);
	skip_header_draw = false;

	JE_initPlayerData();
	if (opt_level == 255)
	{
		do
			opt_level = mt_rand() % COUNTOF(official_levels);
		while (!episodeAvail[official_levels[opt_level].ep]);
	}
	JE_initEpisode(official_levels[opt_level].ep);
	mainLevel = official_levels[opt_level].lev;

	if (opt_ship[0] != 254)
	{
		if (opt_ship[0] == 255)
			opt_ship[0] = mt_rand() % SHIPORDER_NOSECRET;
		player[0].items.ship = shiporder[opt_ship[0]];
		PL_Init(&player[0], player[0].items.ship, true);

		if (opt_weapon[0] == 255)
			opt_weapon[0] = mt_rand() % 5;
		player[0].cur_weapon = opt_weapon[0];
		player[0].items.weapon[0].id = ships[player[0].items.ship].port_weapons[player[0].cur_weapon];

		if (opt_power[0] == 255)
			opt_power[0] = mt_rand() % 11;
		player[0].items.weapon[0].power = opt_power[0] + 1;

		if (opt_shield[0] == 255)
			opt_shield[0] = mt_rand() % 10;
		player[0].items.shield = opt_shield[0] + 1;

		player[0].lives = opt_lives[0];
	}
	if (opt_ship[1] != 254)
	{
		if (opt_ship[1] == 255)
		{
			do
				opt_ship[1] = mt_rand() % SHIPORDER_MAX;
			while (opt_ship[0] == opt_ship[1]);
		}
		player[1].items.ship = shiporder[opt_ship[1]];
		PL_Init(&player[1], player[1].items.ship, true);

		if (opt_weapon[1] == 255)
			opt_weapon[1] = mt_rand() % 5;
		player[1].cur_weapon = opt_weapon[1];
		player[1].items.weapon[0].id = ships[player[1].items.ship].port_weapons[player[1].cur_weapon];

		if (opt_power[1] == 255)
			opt_power[1] = mt_rand() % 11;
		player[1].items.weapon[0].power = opt_power[1] + 1;

		if (opt_shield[1] == 255)
			opt_shield[1] = mt_rand() % 10;
		player[1].items.shield = opt_shield[1] + 1;

		player[1].lives = opt_lives[1];
	}

	demoDifficulty = opt_difficulty;

	superArcadeMode = 1;
	onePlayerAction = true;
}

void DEV_RecordDemoStart( void )
{
	if (new_demo_num == 0)
		++new_demo_num;
	do
	{
		sprintf(tmpBuf.l, "arcdemo.%d", new_demo_num++);
	}
	while (dir_file_exists(get_user_directory(), tmpBuf.l)); // until file doesn't exist

	demo_file = dir_fopen_warn(get_user_directory(), tmpBuf.l, "wb");
	if (!demo_file)
		exit(1);

	efwrite("TAVDM01",   1,  7, demo_file);
	efwrite(&episodeNum, 1,  1, demo_file);
	efwrite(levelName,   1, 10, demo_file);
	efwrite(&lvlFileNum, 1,  1, demo_file);
	efwrite(&levelSong,  1,  1, demo_file);

	Uint64 seed = time(NULL);
	efwrite(&seed, sizeof(Uint64),  1, demo_file);
	mt_srand(seed);

	fputc(demoDifficulty,                  demo_file);
	fputc(PL_WhosInGame(),                 demo_file);
	fputc(player[0].items.ship,            demo_file);
	fputc(player[0].items.shield,          demo_file);
	fputc(player[0].items.sidekick[0],     demo_file);
	fputc(player[0].items.sidekick[1],     demo_file);
	fputc(player[0].lives,                 demo_file);
	fputc(player[0].weapon_mode,           demo_file);
	fputc(player[0].cur_weapon,            demo_file);
	fputc(player[0].items.weapon[0].power, demo_file);
	fputc(player[1].items.ship,            demo_file);
	fputc(player[1].items.shield,          demo_file);
	fputc(player[1].items.sidekick[0],     demo_file);
	fputc(player[1].items.sidekick[1],     demo_file);
	fputc(player[1].lives,                 demo_file);
	fputc(player[1].weapon_mode,           demo_file);
	fputc(player[1].cur_weapon,            demo_file);
	fputc(player[1].items.weapon[0].power, demo_file);

	memset(player[0].buttons, 0, sizeof(player[0].buttons));
	memset(player[1].buttons, 0, sizeof(player[0].buttons));
	memset(player[0].last_buttons, 0, sizeof(player[0].last_buttons));
	memset(player[1].last_buttons, 0, sizeof(player[0].last_buttons));

	demo_record_time = SDL_GetTicks() + (opt_timer * 1000);
}

void DEV_RecordDemoInput( void )
{
	Uint8 temp;

	for (int p = 0; p < 2; ++p)
	{
		temp = 0;
		for (int i = 0; i < 7; ++i)
		{
			if (player[p].buttons[i])
				temp |= 1 << i;
		}
		fputc(temp, demo_file);
	}
}

void DEV_RecordDemoEnd( void )
{
	fclose(demo_file);
	printf("Demo closed.\n");
}
