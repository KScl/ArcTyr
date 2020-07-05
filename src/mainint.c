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
#include "backgrnd.h"
#include "config.h"
#include "episodes.h"
#include "file.h"
#include "fonthand.h"
#include "helptext.h"
#include "helptext.h"
#include "input.h"
#include "lds_play.h"
#include "loudness.h"
#include "mainint.h"
#include "menus.h"
#include "mouse.h"
#include "nortsong.h"
#include "nortvars.h"
#include "opentyr.h"
#include "palette.h"
#include "params.h"
#include "pcxmast.h"
#include "picload.h"
#include "player.h"
#include "shots.h"
#include "sndmast.h"
#include "sprite.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

#include "lib/mtrand.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#define MAX_PAGE 8
#define TOPICS 6
const JE_byte topicStart[TOPICS] = { 0, 1, 2, 3, 7, 255 };

JE_shortint constantLastX;
JE_word textErase;
JE_word upgradeCost;
JE_word downgradeCost;
JE_boolean performSave;
JE_boolean jumpSection;
JE_boolean useLastBank; /* See if I want to use the last 16 colors for DisplayText */

int cameraXFocus;

bool pause_pressed = false, ingamemenu_pressed = false;

/* Draws a message at the bottom text window on the playing screen */
void JE_drawTextWindow( const char *text )
{
	if (textErase > 0) // erase current text
		blit_sprite(VGAScreenSeg, 16, 189, OPTION_SHAPES, 36);  // in-game text area

	textErase = 100;
	JE_outText(VGAScreenSeg, 20, 190, text, 0, 4);
}

void JE_drawTextWindowColorful( const char *text )
{
	char buf[256] = "";
	char *p = buf;
	JE_byte color = 0, brightness = 4;
	JE_integer x = 20;

	if (textErase > 0) // erase current text
		blit_sprite(VGAScreenSeg, 16, 189, OPTION_SHAPES, 36);  // in-game text area

	textErase = 100;

	while (true)
	{
		while (*text && *text != '^')
			*p++ = *text++;
		if (strlen(buf) > 0)
		{
			JE_outText(VGAScreenSeg, x, 190, buf, color, brightness);
			x += JE_textWidth(buf, TINY_FONT);
			memset(buf, 0, sizeof(buf));
			p = buf;
		}
		if (!*text)
			break;
		++text;
		color = (*text >= 'A') ? *text + 10 - 'A' : *text - '0';
		++text;
		brightness = (*text >= 'A') ? *text + 10 - 'A' : *text - '0';
		++text;
	}
}

void JE_colorFromPWeapon( uint pw, JE_byte *c, JE_byte *bri )
{
	switch (pw)
	{
		case 0:  *c = 7;  *bri = 2; break;
		case 1:  *c = 9;  *bri = 5; break;
		case 2:  *c = 0;  *bri = 6; break;
		case 3:  *c = 12; *bri = 5; break;
		case 4:  *c = 15; *bri = 6; break;
		default: *c = 0;  *bri = 4; break;
	}
}

char *JE_textColorFromPWeapon( uint pw )
{
	static char colorStr[4] = "^00";
	switch (pw)
	{
		case 0:  colorStr[1] = '7'; colorStr[2] = '2'; break;
		case 1:  colorStr[1] = '9'; colorStr[2] = '5'; break;
		case 2:  colorStr[1] = '0'; colorStr[2] = '6'; break;
		case 3:  colorStr[1] = 'C'; colorStr[2] = '5'; break;
		case 4:  colorStr[1] = 'F'; colorStr[2] = '6'; break;
		default: colorStr[1] = '0'; colorStr[2] = '4'; break;
	}
	return colorStr;
}

char *JE_trim( const char *str )
{
	static char trimStr[128];
	char *p = trimStr;
	snprintf(trimStr, sizeof(trimStr), "%s", str);
	p += strlen(trimStr) - 1;
	while (p > trimStr && *p == ' ')
		--p;
	++p;
	*p = 0;
	return trimStr;
}

void JE_outCharGlow( JE_word x, JE_word y, const char *s )
{
	JE_integer maxloc, loc, z;
	JE_shortint glowcol[60]; /* [1..60] */
	JE_shortint glowcolc[60]; /* [1..60] */
	JE_word textloc[60]; /* [1..60] */
	JE_byte bank;

	setjasondelay2(1);

	bank = (warningRed) ? 7 : ((useLastBank) ? 15 : 14);

	if (s[0] == '\0')
		return;

	if (frameCountMax == 0)
	{
		JE_textShade(VGAScreen, x, y, s, bank, 0, PART_SHADE);
		JE_showVGA();
	}
	else
	{
		maxloc = strlen(s);
		for (z = 0; z < 60; z++)
		{
			glowcol[z] = -8;
			glowcolc[z] = 1;
		}

		loc = x;
		for (z = 0; z < maxloc; z++)
		{
			textloc[z] = loc;

			int sprite_id = font_ascii[(unsigned char)s[z]];

			if (s[z] == ' ')
				loc += 6;
			else if (sprite_id != -1)
				loc += sprite(TINY_FONT, sprite_id)->width + 1;
		}

		for (loc = 0; (unsigned)loc < strlen(s) + 28; loc++)
		{
			if (true /* !ESCPressed */)
			{
				setjasondelay(frameCountMax);

				int sprite_id = -1;

				for (z = loc - 28; z <= loc; z++)
				{
					if (z >= 0 && z < maxloc)
					{
						sprite_id = font_ascii[(unsigned char)s[z]];

						if (sprite_id != -1)
						{
							blit_sprite_hv(VGAScreen, textloc[z], y, TINY_FONT, sprite_id, bank, glowcol[z]);

							glowcol[z] += glowcolc[z];
							if (glowcol[z] > 9)
								glowcolc[z] = -1;
						}
					}
				}
				if (sprite_id != -1 && --z < maxloc)
					blit_sprite_dark(VGAScreen, textloc[z] + 1, y + 1, TINY_FONT, sprite_id, true);

				if (I_anyButton())
					frameCountMax = 0;

				do
				{
					if (levelWarningDisplay)
						JE_updateWarning(VGAScreen);

					SDL_Delay(16);
				}
				while (!(delaycount() == 0));

				JE_showVGA();
			}
		}
	}
}

void JE_drawPortConfigButtons( void ) // rear weapon pattern indicator
{
	int x, wm;
	static const uint pcbshapes[] = {0, 18, 19, 18, 19, 19};

	for (int i = 0; i < 2; ++i)
	{
		if (player[i].player_status != STATUS_INGAME)
			continue;

		wm = (player[i].items.special) ? player[i].weapon_mode : 4;
		x = (i == 0) ? 16 : 294;

		blit_sprite(VGAScreenSeg, x, 1, OPTION_SHAPES, pcbshapes[wm++]);
		blit_sprite(VGAScreenSeg, x, 10, OPTION_SHAPES, pcbshapes[wm]);		
	}
}

void JE_initPlayerData( void )
{
	player[0].player_status = STATUS_NONE;
	player[1].player_status = STATUS_NONE;
	player[0].cashForNextLife = 50000;
	player[1].cashForNextLife = 50000;
	player[0].is_dragonwing = false;
	player[1].is_dragonwing = false;

	/* JE: New Game Items/Data */

	player[0].items.ship = shiporder[0];
	player[0].items.weapon[FRONT_WEAPON].id = 1;  // Pulse Cannon
	player[0].items.weapon[REAR_WEAPON].id = 0;   // None
	player[0].items.shield = 1;                   // Gencore High Energy Shield
	player[0].items.generator = 2;                // Advanced MR-12
	for (uint i = 0; i < COUNTOF(player[0].items.sidekick); ++i)
		player[0].items.sidekick[i] = 0;          // None
	player[0].items.special = 0;                  // None

	player[0].last_items = player[0].items;

	player[1].items.ship = shiporder[1];
	player[1].items = player[0].items;
	player[1].items.weapon[FRONT_WEAPON].id = 15; // Vulcan Cannon
	player[1].items.sidekick_level = 101;         // 101, 102, 103
	player[1].items.sidekick_series = 0;          // None

	player[0].last_opt_given = player[0].last_opt_fired = 0;
	player[1].last_opt_given = player[1].last_opt_fired = 0;

	superTyrian = false;

	secretHint = (mt_rand() % 3) + 1;

	for (uint p = 0; p < COUNTOF(player); ++p)
	{
		for (uint i = 0; i < COUNTOF(player->items.weapon); ++i)
		{
			player[p].items.weapon[i].power = 1;
		}

		player[p].weapon_mode = 1;
		player[p].armor = 1;
		player[p].lives = 1;
	}

	mainLevel = FIRST_LEVEL;
	saveLevel = FIRST_LEVEL;

	strcpy(lastLevelName, miscText[19]);
}

void JE_gammaCorrect_func( JE_byte *col, JE_real r )
{
	int temp = roundf(*col * r);
	if (temp > 255)
	{
		temp = 255;
	}
	*col = temp;
}

void JE_gammaCorrect( Palette *colorBuffer, JE_byte gamma )
{
	int x;
	JE_real r = 1 + (JE_real)gamma / 10;

	for (x = 0; x < 256; x++)
	{
		JE_gammaCorrect_func(&(*colorBuffer)[x].r, r);
		JE_gammaCorrect_func(&(*colorBuffer)[x].g, r);
		JE_gammaCorrect_func(&(*colorBuffer)[x].b, r);
	}
}

JE_boolean JE_gammaCheck( void )
{
	bool temp = I_KEY_pressed(SDLK_F11);
	if (temp)
	{
		gammaCorrection = (gammaCorrection + 1) % 4;
		memcpy(colors, palettes[pcxpal[3-1]], sizeof(colors));
		JE_gammaCorrect(&colors, gammaCorrection);
		set_palette(colors, 0, 255);
	}
	return temp;
}

void JE_doInGameSetup( void )
{
	if (JE_inGameSetup())
	{
		reallyEndLevel = true;
		playerEndLevel = true;
	}

	//skipStarShowVGA = true;
}

JE_boolean JE_inGameSetup( void )
{
	SDL_Surface *temp_surface = VGAScreen;
	VGAScreen = VGAScreenSeg; /* side-effect of game_screen */

	JE_boolean returnvalue = false;

	const JE_byte help[6] /* [1..6] */ = {15, 15, 28, 29, 26, 27};
	JE_byte  sel;
	JE_boolean quit;

	bool first = true;

	//tempScreenSeg = VGAScreenSeg; /* <MXD> ? should work as VGAScreen */

	quit = false;
	sel = 1;

	JE_barShade(VGAScreen, 3, 13, 217, 137); /*Main Box*/
	JE_barShade(VGAScreen, 5, 15, 215, 135);

	JE_barShade(VGAScreen, 3, 143, 257, 157); /*Help Box*/
	JE_barShade(VGAScreen, 5, 145, 255, 155);
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	do
	{
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (x = 0; x < 6; x++)
		{
			JE_outTextAdjust(VGAScreen, 10, (x + 1) * 20, inGameText[x], 15, ((sel == x+1) << 1) - 4, SMALL_FONT_SHAPES, true);
		}

		JE_outTextAdjust(VGAScreen, 120, 3 * 20, detailLevel[processorType-1], 15, ((sel == 3) << 1) - 4, SMALL_FONT_SHAPES, true);
		JE_outTextAdjust(VGAScreen, 120, 4 * 20, gameSpeedText[gameSpeed-1],   15, ((sel == 4) << 1) - 4, SMALL_FONT_SHAPES, true);

		JE_outTextAdjust(VGAScreen, 10, 147, mainMenuHelp[help[sel-1]-1], 14, 6, TINY_FONT, true);

		JE_barDrawShadow(VGAScreen, 120, 20, 1, music_disabled ? 12 : 16, tyrMusicVolume / 12, 3, 13);
		JE_barDrawShadow(VGAScreen, 120, 40, 1, samples_disabled ? 12 : 16, fxVolume / 12, 3, 13);

		JE_showVGA();

		if (first)
		{
			first = false;
			//wait_noinput(false, false, true); // TODO: should up the joystick repeat temporarily instead
		}

		tempW = 0;
		uint button = INPUT_P1_UP;
		I_waitOnInputForMenu(button, INPUT_P1_FIRE, 0);
		while (I_inputForMenu(&button, INPUT_P1_MODE))
		{
			switch (button++)
			{
			case INPUT_P1_FIRE:
				JE_playSampleNum(S_SELECT);
				switch (sel)
				{
					case 1:
						music_disabled = !music_disabled;
						break;
					case 2:
						samples_disabled = !samples_disabled;
						break;
					case 3:
					case 4:
						sel = 5;
						break;
					case 5:
						quit = true;
						break;
					case 6:
						returnvalue = true;
						quit = true;
						break;
				}
				break;
			case INPUT_P1_UP:
				if (--sel < 1)
				{
					sel = 6;
				}
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_DOWN:
				if (++sel > 6)
				{
					sel = 1;
				}
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_LEFT:
				switch (sel)
				{
					case 1:
						JE_changeVolume(&tyrMusicVolume, -12, &fxVolume, 0);
						if (music_disabled)
						{
							music_disabled = false;
							restart_song();
						}
						break;
					case 2:
						JE_changeVolume(&tyrMusicVolume, 0, &fxVolume, -12);
						samples_disabled = false;
						break;
					case 3:
						if (--processorType < 1)
						{
							processorType = 4;
						}
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
					case 4:
						if (--gameSpeed < 1)
						{
							gameSpeed = 5;
						}
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
				}
				if (sel < 5)
				{
					JE_playSampleNum(S_CURSOR);
				}
				break;
			case INPUT_P1_RIGHT:
				switch (sel)
				{
					case 1:
						JE_changeVolume(&tyrMusicVolume, 12, &fxVolume, 0);
						if (music_disabled)
						{
							music_disabled = false;
							restart_song();
						}
						break;
					case 2:
						JE_changeVolume(&tyrMusicVolume, 0, &fxVolume, 12);
						samples_disabled = false;
						break;
					case 3:
						if (++processorType > 4)
						{
							processorType = 1;
						}
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
					case 4:
						if (++gameSpeed > 5)
						{
							gameSpeed = 1;
						}
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
				}
				if (sel < 5)
				{
					JE_playSampleNum(S_CURSOR);
				}
				break;
			}
		}

//				quit = true;
//				JE_playSampleNum(S_SPRING);
//				break;

	} while (!quit);

	VGAScreen = temp_surface; /* side-effect of game_screen */

	return returnvalue;
}

bool load_next_demo( void )
{
	snprintf(tmpBuf.s, sizeof(tmpBuf.s), "arcdemo.%d", ++demo_num);
	demo_file = dir_fopen_warn(arcdata_dir(), tmpBuf.s, "rb");
	if (!demo_file)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "arcdemo.%d", demo_num = 1);
		demo_file = dir_fopen_warn(arcdata_dir(), tmpBuf.s, "rb");
	}
	if (!demo_file)
		return false;

	printf("loaded demo '%s'\n", tmpBuf.s);

	efread(tmpBuf.s, 1, 7, demo_file);
	if (strncmp(tmpBuf.s, "TAVDM01", 7))
		fprintf(stderr, "warning: wrong demo version\n");

	difficultyLevel = 2; // TODO
	bonusLevelCurrent = false;

	Uint8 temp = fgetc(demo_file);
	JE_initEpisode(temp);
	efread(levelName, 1, 10, demo_file); levelName[10] = '\0';
	lvlFileNum = fgetc(demo_file);
	levelSong = fgetc(demo_file);

	// Don't immediately seed
	efread(&demo_seed, sizeof(Uint64), 1, demo_file);

	demoDifficulty = fgetc(demo_file);

	temp = fgetc(demo_file);

	player[0].items.ship              = fgetc(demo_file);
	if (temp & 1)
		PL_Init(&player[0], player[0].items.ship, true);
	player[0].items.shield            = fgetc(demo_file);
	player[0].items.sidekick[0]       = fgetc(demo_file);
	player[0].items.sidekick[1]       = fgetc(demo_file);
	player[0].lives                   = fgetc(demo_file);
	player[0].weapon_mode             = fgetc(demo_file);
	player[0].cur_weapon              = fgetc(demo_file);
	player[0].items.weapon[0].power   = fgetc(demo_file);
	player[0].cash                    = 0;

	if (temp & 1)
	{
		player[0].items.weapon[0].id = ships[player[0].items.ship].port_weapons[player[0].cur_weapon];
		player[0].items.special = ships[player[0].items.ship].special_weapons[player[0].weapon_mode - 1];
	}

	player[1].items.ship              = fgetc(demo_file);
	if (temp & 2)
		PL_Init(&player[1], player[1].items.ship, true);
	player[1].items.shield            = fgetc(demo_file);
	player[1].items.sidekick[0]       = fgetc(demo_file);
	player[1].items.sidekick[1]       = fgetc(demo_file);
	player[1].lives                   = fgetc(demo_file);
	player[1].weapon_mode             = fgetc(demo_file);
	player[1].cur_weapon              = fgetc(demo_file);
	player[1].items.weapon[0].power   = fgetc(demo_file);
	player[1].cash                    = 0;

	if (temp & 2)
	{
		player[1].items.weapon[0].id = ships[player[1].items.ship].port_weapons[player[1].cur_weapon];
		player[1].items.special = ships[player[1].items.ship].special_weapons[player[1].weapon_mode - 1];
	}

	onePlayerAction = true;

	demo_keys[0] = demo_keys[1] = 0;

	if (DIP.attractSound == 1)
		attractAudioAllowed = (SDL_GetTicks() - attractTic < 300000);
	else
		attractAudioAllowed = (DIP.attractSound == 2);

	return true;
}

/*Street Fighter codes*/
void JE_SFCodes( JE_byte playerNum_ )
{
	JE_byte twidDir, twidCheckDirs;
	JE_byte currentKey;
	JE_byte i;

	Player *this_player = &player[playerNum_-1];
	uint ship = player[playerNum_-1].items.ship;

	// this uses buttons instead of PX/Y and mouseX/Y
	// possibly more precise than before?
	twidCheckDirs = (this_player->buttons[BUTTON_UP])   +  /*UP*/
	                (this_player->buttons[BUTTON_DOWN]) +  /*DOWN*/
	                (this_player->buttons[BUTTON_LEFT]) +  /*LEFT*/
	                (this_player->buttons[BUTTON_RIGHT]);  /*RIGHT*/
	twidDir = (this_player->buttons[BUTTON_UP])    * 1 + /*UP*/
	          (this_player->buttons[BUTTON_DOWN])  * 2 + /*DOWN*/
	          (this_player->buttons[BUTTON_LEFT])  * 3 + /*LEFT*/
	          (this_player->buttons[BUTTON_RIGHT]) * 4;  /*RIGHT*/

	if (twidDir == 0) // no direction being pressed
	{
		if (!this_player->buttons[BUTTON_FIRE]) // if fire button is released
		{
			twidDir = 9;
			twidCheckDirs = 1;
		} else {
			twidCheckDirs = 0;
			twidDir = 99;
		}
	}

	if (twidCheckDirs == 1) // if exactly one direction pressed or firebutton is released
	{
		twidDir += this_player->buttons[BUTTON_FIRE] * 4;
		// 1: UP      5: UP+FIRE
		// 2: DOWN    6: DOWN+FIRE
		// 3: LEFT    7: LEFT+FIRE
		// 4: RIGHT   8: RIGHT+FIRE
		// 9: All buttons and directions released

		for (i = 0; i < ships[ship].numTwiddles; i++)
		{
			// get next combo key
			currentKey = ships[ship].twiddles[i][SFCurrentCode[playerNum_-1][i]];

			// correct key
			if (currentKey == twidDir)
			{
				SFCurrentCode[playerNum_-1][i]++;

				currentKey = ships[ship].twiddles[i][SFCurrentCode[playerNum_-1][i]];
				if (currentKey > 100 && currentKey <= 100 + SPECIAL_NUM)
				{
					SFCurrentCode[playerNum_-1][i] = 0;
					SFExecuted[playerNum_-1] = currentKey - 100;
				}
			}
			else
			{
				if ((twidDir != 9) &&
				    (currentKey - 1) % 4 != (twidDir - 1) % 4 &&
				    (SFCurrentCode[playerNum_-1][i] == 0 ||
				     ships[ship].twiddles[i][SFCurrentCode[playerNum_-1][i]-1] != twidDir))
				{
					SFCurrentCode[playerNum_-1][i] = 0;
				}
			}
		}
	}
}

void JE_playCredits( void )
{
	static const JE_byte ships_allowed[] = {2, 1, 3, 4, 5, 6, 2};
	int i;
	printf("Now playing credits\n");

	playingCredits = true;

	for (i = 0; i < 2; ++i)
	{
		if (player[i].player_status == STATUS_INGAME)
			ARC_SetPlayerStatus(&player[i], STATUS_NAMEENTRY);
		else if (player[i].player_status != STATUS_NAMEENTRY)
			ARC_SetPlayerStatus(&player[i], STATUS_GAMEOVER);
	}

	enum { lines_max = 132 };
	enum { line_max_length = 65 };
	
	char credstr[lines_max][line_max_length + 1];
	
	int lines = 0;
	
	JE_byte currentpic = 0, fade = 0;
	JE_shortint fadechg = 1;
	JE_byte currentship = 0;
	JE_integer shipx = 0, shipxwait = 0;
	JE_shortint shipxc = 0, shipxca = 0;

	load_sprites_file(EXTRA_SHAPES, data_dir(), "estsc.shp");

	setjasondelay2(1000);

	play_song(8);
	
	// load credits text
	FILE *f = dir_fopen_die(data_dir(), "tyrian.cdt", "rb");
	for (lines = 0; !feof(f) && lines < lines_max; ++lines)
	{
		read_encrypted_pascal_string(credstr[lines], sizeof(credstr[lines]), f);
		if (!strncmp(credstr[lines]+1, "Tim Sweeney", 11))
			strncpy(credstr[lines]+1, "Hatsune Miku", line_max_length-1);
	}
	if (lines == lines_max)
		--lines;
	fclose(f);
	
	memcpy(colors, palettes[6-1], sizeof(colors));
	JE_clr256(VGAScreen);
	JE_showVGA();
	fade_palette(colors, 2, 0, 255);
	
	//tempScreenSeg = VGAScreenSeg;
	
	const int ticks_max = lines * 20 * 3;
	for (int ticks = 0; ticks < ticks_max; ++ticks)
	{
		setjasondelay(1);
		JE_clr256(VGAScreen);
		
		blit_sprite_hv(VGAScreenSeg, 319 - sprite(EXTRA_SHAPES, currentpic)->width, 100 - (sprite(EXTRA_SHAPES, currentpic)->height / 2), EXTRA_SHAPES, currentpic, 0x0, fade - 15);
		
		fade += fadechg;
		if (fade == 0 && fadechg == -1)
		{
			fadechg = 1;
			++currentpic;
			if (currentpic >= sprite_table[EXTRA_SHAPES].count)
				currentpic = 0;
		}
		if (fade == 15)
			fadechg = 0;

		if (delaycount2() == 0)
		{
			fadechg = -1;
			setjasondelay2(900);
		}

		if (ticks % 200 == 0)
		{
			currentship = ships_allowed[mt_rand() % COUNTOF(ships_allowed)];
			shipxwait = (mt_rand() % 80) + 10;
			if ((mt_rand() % 2) == 1)
			{
				shipx = 1;
				shipxc = 0;
				shipxca = 1;
			}
			else
			{
				shipx = 900;
				shipxc = 0;
				shipxca = -1;
			}
		}

		shipxwait--;
		if (shipxwait == 0)
		{
			if (shipx == 1 || shipx == 900)
				shipxc = 0;
			shipxca = -shipxca;
			shipxwait = (mt_rand() % 40) + 15;
		}
		shipxc += shipxca;
		shipx += shipxc;
		if (shipx < 1)
		{
			shipx = 1;
			shipxwait = 1;
		}
		if (shipx > 900)
		{
			shipx = 900;
			shipxwait = 1;
		}
      	int tmp_unknown = shipxc * shipxc;
		if (450 + tmp_unknown < 0 || 450 + tmp_unknown > 900)
		{
			if (shipxca < 0 && shipxc < 0)
				shipxwait = 1;
			if (shipxca > 0 && shipxc > 0)
				shipxwait = 1;
		}
		
		uint ship_sprite = ships[currentship].shipgraphic;
		if (shipxc < -10)
			ship_sprite -= (shipxc < -20) ? 4 : 2;
		else if (shipxc > 10)
			ship_sprite += (shipxc > 20) ? 4 : 2;
		
		blit_sprite2x2(VGAScreen, shipx / 40, 184 - (ticks % 200), shapes9, ship_sprite);
		
		const int bottom_line = (ticks / 3) / 20;
		int y = 20 - ((ticks / 3) % 20);
		
		for (int line = bottom_line - 10; line < bottom_line; ++line)
		{
			if (line >= 0 && line < lines_max)
			{
				if (strcmp(&credstr[line][0], ".") != 0 && strlen(credstr[line]))
				{
					const Uint8 color = credstr[line][0] - 65;
					const char *text = &credstr[line][1];
					
					const int x = 110 - JE_textWidth(text, SMALL_FONT_SHAPES) / 2;
					
					JE_outTextAdjust(VGAScreen, x + abs((y / 18) % 4 - 2) - 1, y - 1, text, color, -8, SMALL_FONT_SHAPES, false);
					JE_outTextAdjust(VGAScreen, x,                             y,     text, color, -2, SMALL_FONT_SHAPES, false);
				}
			}
			
			y += 20;
		}
		
		fill_rectangle_xy(VGAScreen, 0,  0, 319, 10, 0);
		fill_rectangle_xy(VGAScreen, 0, 190, 319, 199, 0);
		
		if (currentpic == sprite_table[EXTRA_SHAPES].count - 1)
			JE_outTextAdjust(VGAScreen, 5, 180, miscText[54], 2, -2, SMALL_FONT_SHAPES, false);  // levels-in-episode
		
		if (bottom_line == lines_max - 8)
			fade_song();
		
		if (ticks == ticks_max - 1)
		{
			--ticks;
			play_song(9);
		}
		
		// Credits runs at 70fps; only handle input half the time
		if (!(ticks & 1))
		{
			I_checkButtons();
			ARC_HandlePlayerStatus(&player[0], 1);
			ARC_HandlePlayerStatus(&player[1], 2);
		}

		JE_showVGA();
		
		wait_delay();
		
		if (PL_NumPotentialPlayers() == 0 && I_anyButton())
			break;
	}
	
	playingCredits = false;

	fade_black(10);
	
	load_sprites_file(EXTRA_SHAPES, arcdata_dir(), "arcopts.spr");
}

static void award_bonus_points( uint points )
{
	if (player[0].player_status == STATUS_INGAME)
	{
		player[0].cash += points;
		ARC_ScoreLife(&player[0]);
	}
	if (player[1].player_status == STATUS_INGAME)
	{
		player[1].cash += points;
		ARC_ScoreLife(&player[1]);
	}
	if (soundQueue[6])
		JE_playSampleNumOnChannel(soundQueue[6], 6);
	if (soundQueue[7])
		JE_playSampleNumOnChannel(soundQueue[7], 7);
}

void JE_endLevelAni( void )
{
	int destruct1 = 0, destruct2 = 0;

	// We don't use tmpBuf because this function needs strings to persist beyond screen flips
	char tempStr[256];

	if (allPlayersGone)
		play_song(SONG_GAMEOVER);
	else //if (!normalBonusLevelCurrent || !bonusLevelCurrent)
		ARC_RankIncrease();	

	player[0].last_items = player[0].items;
	strcpy(lastLevelName, levelName);

	frameCountMax = 4;
	textGlowFont = SMALL_FONT_SHAPES;

	SDL_Color white = { 255, 255, 255 };
	set_colors(white, 254, 254);

	if (episodeNum == 4 && levelTimer && (levelTimerCountdown <= 0 || allPlayersGone))
		play_song(21);
	else
		JE_playSampleNumOnChannel(V_LEVEL_END, SFXPRIORITY + 2);	

	// ---

	if (bonusLevel)
		strcpy(tempStr, miscText[17-1]);
	else if (episodeNum == 4 && levelTimer && (levelTimerCountdown <= 0 || allPlayersGone))
		sprintf(tempStr, "%s %s", "Failed:", levelName);
	else if (!allPlayersGone)
		sprintf(tempStr, "%s %s", miscText[27-1], levelName); // "Completed"
	else
		sprintf(tempStr, "%s %s", miscText[62-1], levelName); // "Exiting"

	JE_outTextGlow(VGAScreenSeg, 48, 20, tempStr);

	// ---

	JE_saveTextGlow(JE_fontCenter("Destruction", SMALL_FONT_SHAPES), 40, "Destruction");

	if (player[0].player_status == STATUS_INGAME)
	{
		destruct1 = (totalEnemy == 0) ? 0 : roundf(enemyKilled[0] * 100 / totalEnemy);
		sprintf(tempStr, "%d%%", destruct1);
		JE_saveTextGlow(50, 55, tempStr);
	}

	if (player[1].player_status == STATUS_INGAME)
	{
		destruct2 = (totalEnemy == 0) ? 0 : roundf(enemyKilled[1] * 100 / totalEnemy);
		sprintf(tempStr, "%d%%", destruct2);
		JE_saveTextGlow(270 - JE_textWidth(tempStr, SMALL_FONT_SHAPES), 55, tempStr);
	}

	JE_drawTextGlow(VGAScreenSeg);

	// ---

	JE_saveTextGlow(JE_fontCenter("Destruction bonus.", SMALL_FONT_SHAPES), 80, "Destruction bonus.");

	if (player[0].player_status == STATUS_INGAME)
	{
		destruct1 *= 100;
		player[0].cash += destruct1;
		ARC_ScoreLife(&player[0]);

		sprintf(tempStr, "%d", destruct1);
		JE_saveTextGlow(50, 95, tempStr);
	}

	if (player[1].player_status == STATUS_INGAME)
	{
		destruct2 *= 100;
		player[1].cash += destruct2;
		ARC_ScoreLife(&player[1]);

		sprintf(tempStr, "%d", destruct2);
		JE_saveTextGlow(270 - JE_textWidth(tempStr, SMALL_FONT_SHAPES), 95, tempStr);
	}

	if (soundQueue[6])
		JE_playSampleNumOnChannel(soundQueue[6], 6);
	if (soundQueue[7])
		JE_playSampleNumOnChannel(soundQueue[7], 7);

	JE_drawTextGlow(VGAScreenSeg);

	// ---

	// Bonuses (currently hardcoded)
	//printf("%d %d\n", episodeNum, mainLevel);

	if (episodeNum == 4 && mainLevel == 32 && !allPlayersGone && !(levelTimer && levelTimerCountdown <= 0))
	{
		strcpy(tempStr, "Bonus for saving Ixmucane.");
		JE_saveTextGlow(JE_fontCenter(tempStr, SMALL_FONT_SHAPES), 120, tempStr);
		if (player[0].player_status == STATUS_INGAME)
			JE_saveTextGlow(50, 135, "100000");
		if (player[1].player_status == STATUS_INGAME)
			JE_saveTextGlow(270 - JE_textWidth("100000", SMALL_FONT_SHAPES), 135, "100000");
		award_bonus_points(100000);

		JE_drawTextGlow(VGAScreenSeg);
	}
	else if ((episodeNum == 1 && mainLevel == 42)
		|| (episodeNum == 2 && mainLevel == 13)
		|| (episodeNum == 3 && initialDifficulty <= 2 && mainLevel == 16)
		|| (episodeNum == 3 && initialDifficulty > 2 && mainLevel == 13)
		|| (episodeNum == 4 && mainLevel == 37))
	{
		strcpy(tempStr, "Episode completion bonus.");
		JE_saveTextGlow(JE_fontCenter(tempStr, SMALL_FONT_SHAPES), 120, tempStr);
		if (player[0].player_status == STATUS_INGAME)
			JE_saveTextGlow(50, 135, "10000");
		if (player[1].player_status == STATUS_INGAME)
			JE_saveTextGlow(270 - JE_textWidth("10000", SMALL_FONT_SHAPES), 135, "10000");
		award_bonus_points(10000);

		JE_drawTextGlow(VGAScreenSeg);
	}

	// ---

	if (frameCountMax != 0)
	{
		frameCountMax = 6;
		temp = 1;
	} else {
		temp = 0;
	}
	JE_outTextGlow(VGAScreenSeg, JE_fontCenter("Press Fire...--", SMALL_FONT_SHAPES), 160, "Press Fire...");

	do
	{
		setjasondelay(1);

		wait_delay();
	} while (!(I_anyButton() || (frameCountMax == 0 && temp == 1)));

	fade_black(15);
	JE_clr256(VGAScreen);
}

void JE_drawCube( SDL_Surface * screen, JE_word x, JE_word y, JE_byte filter, JE_byte brightness )
{
	blit_sprite_dark(screen, x + 4, y + 4, OPTION_SHAPES, 25, false);
	blit_sprite_dark(screen, x + 3, y + 3, OPTION_SHAPES, 25, false);
	blit_sprite_hv(screen, x, y, OPTION_SHAPES, 25, filter, brightness);
}

bool str_pop_int( char *str, int *val )
{
	bool success = false;

	char buf[256];
	assert(strlen(str) < sizeof(buf));

	// grab the value from str
	char *end;
	*val = strtol(str, &end, 10);

	if (end != str)
	{
		success = true;

		// shift the rest to the beginning
		strcpy(buf, end);
		strcpy(str, buf);
	}

	return success;
}

void JE_inGameDisplays( void )
{
	++arcTextTimer;
	arcTextTimer %= 200;

	// Special Weapon?
	ARC_HUD_ReadyingBar(&player[0], 28);
	ARC_HUD_ReadyingBar(&player[1], 264);

	// Debug Difficulty Display
	if (debug)
	{
		strcpy(tmpBuf.l, difficultyNameB[difficultyLevel + 1]);
		JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 4, tmpBuf.l, 5, 6, FULL_SHADE);
		snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%d / %hu.%hu" , levelAdjustRank, currentRank / 8, currentRank % 8);
		JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 13, tmpBuf.l, 5, 4, FULL_SHADE);
	}
	if (record_demo)
	{
		Uint64 timer = (demo_record_time - SDL_GetTicks()) + 999;
		if (timer % 1000 < 500)
			JE_textShade(VGAScreen, JE_fontCenter(":", TINY_FONT), 4, ":", 5, 6, FULL_SHADE);
		timer /= 1000;
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%02lu", timer / 60);
		JE_textShade(VGAScreen, 159-JE_textWidth(tmpBuf.s, TINY_FONT), 4, tmpBuf.s, 5, 6, FULL_SHADE);
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%02lu", timer % 60);
		JE_textShade(VGAScreen, 161, 4, tmpBuf.s, 5, 6, FULL_SHADE);
	}

	// Lives Left
	for (int temp = 0; temp < 2; temp++)
	{
		if (!player[temp].lives || player[temp].player_status != STATUS_INGAME)
			continue;

		const uint extra_lives = player[temp].lives - 1;

		int y = 166;
		tempW = (temp == 0) ? 32 : 278;

		if (extra_lives >= 5)
		{
			blit_sprite2(VGAScreen, tempW, y, shapes9, 1);
			tempW = (temp == 0) ? 48 : 268;
			sprintf(tmpBuf.s, "%d", extra_lives);
			JE_textShade(VGAScreen, tempW, y + 3, tmpBuf.s, 15, 1, FULL_SHADE);
		}
		else for (uint i = 0; i < extra_lives; ++i)
		{
			blit_sprite2(VGAScreen, tempW, y, shapes9, 1);
			tempW += (temp == 0) ? 12 : -12;
		}
	}

	if (youAreCheating)
	{
		JE_outText(VGAScreen, 90, 170, "Cheaters always prosper.", 3, 4);
	}
}

void JE_mainKeyboardInput( void )
{
	JE_gammaCheck();

	/* { Network Request Commands } */

	// Debug / Cheating
	if (debug)
	{
		if (I_KEY_pressed(SDLK_F2))
			youAreCheating = !youAreCheating;
		if (I_KEY_pressed(SDLK_F3) || I_KEY_pressed(SDLK_F4))
		{
			levelTimer = I_KEY_pressed(SDLK_F3); // F3 for fail, F4 for success
			levelTimerCountdown = 0;
			endLevel = true;
			levelEnd = 40;
		}
	}

	// Enable Debug
	if (I_KEY_pressed(SDLK_BACKSPACE))
	{
		debug = !debug;

		debugHist = 1;
		debugHistCount = 1;

		/* YKS: clock ticks since midnight replaced by SDL_GetTicks */
		lastDebugTime = SDL_GetTicks();
	}

	/* pause game */
	pause_pressed = pause_pressed || I_KEY_pressed(SDLK_p);

	/* in-game setup */
	ingamemenu_pressed = ingamemenu_pressed || I_KEY_pressed(SDLK_ESCAPE);

/*
	if (debug)
	{
		// {SMOOTHIES}
		if (I_KEY_pressed(SDLK_F12))
		{
			for (temp = SDLK_2; temp <= SDLK_9; temp++)
			{
				if (I_KEY_pressed(temp))
					smoothies[temp-SDLK_2] = !smoothies[temp-SDLK_2];
			}
			if (I_KEY_pressed(SDLK_0))
				smoothies[8] = !smoothies[8];
		}

		// {CYCLE THROUGH FILTER COLORS}
		if (I_KEY_pressed(SDLK_MINUS))
		{
			if (levelFilter == -99)
				levelFilter = 0;
			else if (++levelFilter == 16)
				levelFilter = -99;
		}

		// {HYPER-SPEED}
		if (I_KEY_pressed(SDLK_1))
		{
			if (++fastPlay > 2)
				fastPlay = 0;
			JE_setNewGameSpeed();
		}

		// {IN-GAME RANDOM MUSIC SELECTION}
		if (I_KEY_pressed(SDLK_SCROLLOCK))
			play_song(mt_rand() % MUSIC_NUM);
	}
*/
}

void JE_pauseGame( void )
{
	//tempScreenSeg = VGAScreenSeg; // sega000

	set_volume(tyrMusicVolume / 2, fxVolume);

	//wait_noinput(false, false, true); // TODO: should up the joystick repeat temporarily instead

	do
	{
		setjasondelay(2);
		I_checkButtons();
		wait_delay();
	} while (!I_KEY_pressed(SDLK_p));

	set_volume(tyrMusicVolume, fxVolume);

	//skipStarShowVGA = true;
}

void JE_playerMovement( Player *this_player,
                        JE_byte playerNum_,
                        JE_word shipGr_,
                        Sprite2_array *shapes9ptr_,
                        JE_word *mouseX_, JE_word *mouseY_ )
{
	JE_integer mouseXC, mouseYC;
	JE_integer accelXC, accelYC;

	// Always update input
	if (play_demo) // Demo input
		I_demoKeysToInput(this_player, playerNum_);
	else // Game input
		I_assignInput(this_player, playerNum_);

redo:
	mouseXC = 0;
	mouseYC = 0;
	accelXC = 0;
	accelYC = 0;

	/* Draw Player */
	if (!this_player->is_alive)
	{
		if (endLevel)
		{
			// If the level is ending, stop exploding IMMEDIATELY and respawn if we can,
			// so that we can catch the level end warp
			this_player->exploding_ticks = 0;
		}

		if (this_player->exploding_ticks > 0)
		{
			--this_player->exploding_ticks;

			if (deathExplodeSfxWait > 0)
			{
				deathExplodeSfxWait--;
			}
			else
			{
				deathExplodeSfxWait = (mt_rand() % 6) + 3;
				if ((mt_rand() % 3) == 1)
					soundQueue[6] = S_EXPLOSION_9;
				else
					soundQueue[5] = S_EXPLOSION_11;
			}

			int explosion_x = this_player->x + (mt_rand() % 32) - 16;
			int explosion_y = this_player->y + (mt_rand() % 32) - 16;
			JE_setupExplosionLarge(false, 0, explosion_x, explosion_y + 7);
			JE_setupExplosionLarge(false, 0, this_player->x, this_player->y + 7);

			if (levelEnd > 0)
				levelEnd--;
		}
		else
		{
			// finished exploding: check for lives now
			if (normalBonusLevelCurrent || bonusLevelCurrent)
			{
				// null; don't respawn in bonus stages, but don't penalize in any way
			}
			else
			{
				this_player->items.weapon[0].power = 1;
				this_player->shot_multi_pos[SHOT_NORMAL] = 0;
				this_player->shot_repeat[SHOT_NORMAL] = 10;
				this_player->items.shield = 1;		

				if (this_player->shot_repeat[SHOT_SPECIAL] == 0)
				{
					this_player->shot_repeat[SHOT_SPECIAL] = 2;
					this_player->hud_ready_timer = 0;
					this_player->hud_repeat_start = 1;
				}

				if (this_player->lives && --this_player->lives)
				{
					// should be redundant
					//twoPlayerLinked = false;

					this_player->armor = this_player->initial_armor;
					this_player->shield_max = shields[this_player->items.shield].mpwr * 2;
					this_player->shield = this_player->shield_max / 2;

					this_player->y = 160;
					this_player->invulnerable_ticks = 100;
					this_player->is_alive = true;

					JE_drawArmor();
					JE_drawShield();
					goto redo;
				}
				else // just ran out of lives
				{
					// erase armor
					fill_rectangle_xy(VGAScreenSeg, (playerNum_ == 1) ? 16 : 308, 137 - 15, (playerNum_ == 1) ? 24 : 316, 194 - 15, 0);

					// erase shield
					fill_rectangle_xy(VGAScreenSeg, (playerNum_ == 1) ? 3 : 295, 137 - 15, (playerNum_ == 1) ? 11 : 303, 194 - 15, 0);

					// erase power
					fill_rectangle_xy(VGAScreenSeg, (playerNum_ == 1) ?  4 : 308, 8,  (playerNum_ == 1) ? 11 : 315, 62, 0);
					fill_rectangle_xy(VGAScreenSeg, (playerNum_ == 1) ? 15 : 295, 55, (playerNum_ == 1) ? 24 : 304, 62, 0);

					// erase mode buttons
					blit_sprite(VGAScreenSeg, (playerNum_ == 1) ? 16 : 294, 1, OPTION_SHAPES, 19);
					blit_sprite(VGAScreenSeg, (playerNum_ == 1) ? 16 : 294, 10, OPTION_SHAPES, 19);

					// remove player's special (for the header)
					this_player->items.special = 0;
					this_player->is_dragonwing = false;

					// for this frame only: undraw the header after drawing it (we might be about to pause action for a bit)
					skip_header_undraw = false;	
				}
			}
		}
	}

	if (!this_player->is_alive)
	{
		explosionFollowAmountX = explosionFollowAmountY = 0;
		return;
	}

	// ****
	// NOTE: this_player->is_alive checks past this point are REDUNDANT
	// ****

	if (!endLevel)
	{
		*mouseX_ = this_player->x;
		*mouseY_ = this_player->y;

		/* --- Movement Routine Beginning --- */
		if (this_player->buttons[BUTTON_UP])
			this_player->y -= CURRENT_KEY_SPEED;
		if (this_player->buttons[BUTTON_DOWN])
			this_player->y += CURRENT_KEY_SPEED;

		if (this_player->buttons[BUTTON_LEFT])
			this_player->x -= CURRENT_KEY_SPEED;
		if (this_player->buttons[BUTTON_RIGHT])
			this_player->x += CURRENT_KEY_SPEED;

		if (smoothies[9-1])
		{
			*mouseY_ = this_player->y - (*mouseY_ - this_player->y);
			mouseYC = -mouseYC;
		}

		accelXC += this_player->x - *mouseX_;
		accelYC += this_player->y - *mouseY_;

		if (mouseXC > 30)
			mouseXC = 30;
		else if (mouseXC < -30)
			mouseXC = -30;
		if (mouseYC > 30)
			mouseYC = 30;
		else if (mouseYC < -30)
			mouseYC = -30;

		if (mouseXC > 0)
			this_player->x += (mouseXC + 3) / 4;
		else if (mouseXC < 0)
			this_player->x += (mouseXC - 3) / 4;
		if (mouseYC > 0)
			this_player->y += (mouseYC + 3) / 4;
		else if (mouseYC < 0)
			this_player->y += (mouseYC - 3) / 4;

		if (mouseXC > 3)
			accelXC++;
		else if (mouseXC < -2)
			accelXC--;
		if (mouseYC > 2)
			accelYC++;
		else if (mouseYC < -2)
			accelYC--;

		/* --- Movement Routine Ending --- */

		// Street-Fighter codes
		JE_SFCodes(playerNum_);

		// Linking Routines
		twoPlayerLinkReady = (!twoPlayerLinked && PL_Alive(0) && PL_Alive(1)
		                      && abs(player[0].x - player[1].x) < 8
		                      && abs(player[0].y - player[1].y) < 8);

		if (this_player->is_dragonwing && 
			this_player->buttons[BUTTON_MODE] && !this_player->last_buttons[BUTTON_MODE])
		{
			if (twoPlayerLinkReady)
			{
				twoPlayerLinked = true;
				twoPlayerLinkReady = false;	
				JE_playSampleNumOnChannel(S_CLINK, SFXPRIORITY+4);	
				linkGunDirec = M_PI;
				this_player->shot_multi_pos[SHOT_AIMED] = 0;
			}
			else if (twoPlayerLinked)
			{
				twoPlayerLinked = false;
				JE_playSampleNumOnChannel(S_SPRING, SFXPRIORITY+4);				
			}
		}

		// Move the dragonwing's gun
		if (twoPlayerLinked && this_player->is_dragonwing
		    && (this_player->x != *mouseX_ || this_player->y != *mouseY_))
		{
			JE_real tempR;

			if (this_player->x == *mouseX_)
				tempR = (this_player->y - *mouseY_ > 0) ? 0 : M_PI;
			else if (this_player->y == *mouseY_)
				tempR = (this_player->x - *mouseX_ > 0) ? M_PI_2 : (M_PI + M_PI_2);
			else // diagonal
			{
				tempR =  (this_player->x - *mouseX_ > 0) ?  M_PI_4 : (2 * M_PI) - M_PI_4;
				if (this_player->y - *mouseY_ < 0)
					tempR += (this_player->x - *mouseX_ > 0) ?  M_PI_2 : -M_PI_2;
			}

			if (fabsf(linkGunDirec - tempR) < 0.3f)
				linkGunDirec = tempR;
			else if (linkGunDirec < tempR && linkGunDirec - tempR > -3.24f)
				linkGunDirec += 0.2f;
			else if (linkGunDirec - tempR < M_PI)
				linkGunDirec -= 0.2f;
			else
				linkGunDirec += 0.2f;

			if (linkGunDirec >= (2 * M_PI))
				linkGunDirec -= (2 * M_PI);
			else if (linkGunDirec < 0)
				linkGunDirec += (2 * M_PI);
		}
	}

	if (levelEnd > 0 && all_players_dead())
		reallyEndLevel = true;

	/* End Level Fade-Out */
	if (endLevel)
	{
		if (levelEnd == 0)
		{
			reallyEndLevel = true;
			return;
		}
		else
		{
			this_player->y -= levelEndWarp;
			if (this_player->y < -200)
				reallyEndLevel = true;

			int trail_spacing = 1;
			int trail_y = this_player->y;
			int num_trails = abs(41 - levelEnd);
			if (num_trails > 20)
				num_trails = 20;

			for (int i = 0; i < num_trails; i++)
			{
				trail_y += trail_spacing;
				trail_spacing++;
			}

			for (int i = 1; i < num_trails; i++)
			{
				trail_y -= trail_spacing;
				trail_spacing--;

				if (trail_y > 0 && trail_y < 170)
				{
					if (shipGr_ == 0)
					{
						blit_sprite2x2(VGAScreen, this_player->x - 17, trail_y - 7, *shapes9ptr_, 13);
						blit_sprite2x2(VGAScreen, this_player->x + 7 , trail_y - 7, *shapes9ptr_, 51);
					}
					else if (shipGr_ == 1)
					{
						blit_sprite2x2(VGAScreen, this_player->x - 17, trail_y - 7, *shapes9ptr_, 318);
						blit_sprite2x2(VGAScreen, this_player->x + 7 , trail_y - 7, *shapes9ptr_, 320);
					}
					else
					{
						blit_sprite2x2(VGAScreen, this_player->x - 5, trail_y - 7, *shapes9ptr_, shipGr_);
					}
				}
			}
		}

		// all actions past this used to require endLevel be false,
		// there's no point in continuing
		return;
	}

	// ****
	// NOTE: endLevel checks past this point are REDUNDANT
	// ****

	// player movement
	// when linked, the dragonwing has no XY movement controls
	if (!twoPlayerLinked || !this_player->is_dragonwing)
	{
		// non-dragonwing sidekicks
		if (shipGr2 != 0)
		{
			if (this_player->sidekick[LEFT_SIDEKICK].style == 0)
			{
				this_player->sidekick[LEFT_SIDEKICK].x = *mouseX_ - 14;
				this_player->sidekick[LEFT_SIDEKICK].y = *mouseY_;
			}

			if (this_player->sidekick[RIGHT_SIDEKICK].style == 0)
			{
				this_player->sidekick[RIGHT_SIDEKICK].x = *mouseX_ + 16;
				this_player->sidekick[RIGHT_SIDEKICK].y = *mouseY_;
			}
		}

		if (this_player->x_friction_ticks > 0)
		{
			--this_player->x_friction_ticks;
		}
		else
		{
			this_player->x_friction_ticks = 1;

			if (this_player->x_velocity < 0)
				++this_player->x_velocity;
			else if (this_player->x_velocity > 0)
				--this_player->x_velocity;
		}

		if (this_player->y_friction_ticks > 0)
		{
			--this_player->y_friction_ticks;
		}
		else
		{
			this_player->y_friction_ticks = 2;

			if (this_player->y_velocity < 0)
				++this_player->y_velocity;
			else if (this_player->y_velocity > 0)
				--this_player->y_velocity;
		}

		this_player->x_velocity += accelXC;
		this_player->y_velocity += accelYC;

		this_player->x_velocity = MIN(MAX(-4, this_player->x_velocity), 4);
		this_player->y_velocity = MIN(MAX(-4, this_player->y_velocity), 4);

		this_player->x += this_player->x_velocity;
		this_player->y += this_player->y_velocity;

		// if player moved, add new ship x, y history entry
		if (this_player->x - *mouseX_ != 0 || this_player->y - *mouseY_ != 0)
		{
			for (uint i = 1; i < COUNTOF(player->old_x); ++i)
			{
				this_player->old_x[i - 1] = this_player->old_x[i];
				this_player->old_y[i - 1] = this_player->old_y[i];
			}
			this_player->old_x[COUNTOF(player->old_x) - 1] = this_player->x;
			this_player->old_y[COUNTOF(player->old_x) - 1] = this_player->y;
			this_player->moved = true;
		}
		else
			this_player->moved = false;
	}
	else  /*twoPlayerLinked*/
	{
		Player *other_player = PL_OtherPlayer(this_player);

		this_player->x = other_player->x - ((shipGr_ == 0) ? 1 : 0);
		this_player->y = other_player->y + 8;

		this_player->x_velocity = other_player->x_velocity;
		this_player->y_velocity = 4;

		// if the other player moved, update our old x/y positions too
		if (other_player->moved)
		{
			for (uint i = 1; i < COUNTOF(player->old_x); ++i)
			{
				this_player->old_x[i - 1] = this_player->old_x[i];
				this_player->old_y[i - 1] = this_player->old_y[i];
			}
			this_player->old_x[COUNTOF(player->old_x) - 1] = this_player->x;
			this_player->old_y[COUNTOF(player->old_x) - 1] = this_player->y;
			this_player->moved = true;
		}
		else
			this_player->moved = false;

		// turret direction marker/shield
		b = player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec + 0.2f) * 26), this_player->y + roundf(cosf(linkGunDirec + 0.2f) * 26), *mouseX_, *mouseY_, 148, playerNum_);
		b = player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec - 0.2f) * 26), this_player->y + roundf(cosf(linkGunDirec - 0.2f) * 26), *mouseX_, *mouseY_, 148, playerNum_);
		b = player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec) * 26), this_player->y + roundf(cosf(linkGunDirec) * 26), *mouseX_, *mouseY_, 147, playerNum_);

		if (PL_ShotRepeat(this_player, SHOT_AIMED) && this_player->buttons[BUTTON_FIRE])
		{
			const uint item_power = (this_player->items.weapon[FRONT_WEAPON].power - 1) >> 1;

			b = player_shot_create(0, SHOT_AIMED, 
				this_player->x, this_player->y, *mouseX_, *mouseY_, 
				weaponPort[this_player->items.weapon[FRONT_WEAPON].id].aimedOp[item_power], playerNum_);
		}
	}

	if (this_player->x > 256)
	{
		this_player->x = 256;
		constantLastX = -constantLastX;
	}
	if (this_player->x < 40)
	{
		this_player->x = 40;
		constantLastX = -constantLastX;
	}

	if (twoPlayerLinked && !this_player->is_dragonwing)
	{
		if (this_player->y > 154)
			this_player->y = 154;
	}
	else
	{
		if (this_player->y > 160)
			this_player->y = 160;
	}

	if (this_player->y < 10)
		this_player->y = 10;

	// Determines the ship banking sprite to display, depending on horizontal velocity and acceleration
	int ship_banking = this_player->x_velocity / 2 + (this_player->x - *mouseX_) / 6;
	ship_banking = MAX(-2, MIN(ship_banking, 2));

	int ship_sprite = ship_banking * 2 + shipGr_;

	explosionFollowAmountX = this_player->x - this_player->last_x_explosion_follow;
	explosionFollowAmountY = this_player->y - this_player->last_y_explosion_follow;

	if (explosionFollowAmountY < 0)
		explosionFollowAmountY = 0;

	this_player->last_x_explosion_follow = this_player->x;
	this_player->last_y_explosion_follow = this_player->y;

	// Ground shadows
	if (background2)
	{
		if (shipGr_ == 0)
		{
			blit_sprite2x2_darken(VGAScreen, this_player->x - 17 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite + 13);
			blit_sprite2x2_darken(VGAScreen, this_player->x + 7 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite + 51);
		}
		else if (shipGr_ == 1)
		{
			blit_sprite2x2_darken(VGAScreen, this_player->x - 5  - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 319);
			blit_sprite2_darken(VGAScreen,   this_player->x - 17 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 318);
			blit_sprite2_darken(VGAScreen,   this_player->x - 17 - mapX2Ofs + 30, this_player->y + 7 + shadowYDist, *shapes9ptr_, 337 + ((ship_banking > 0) ? -ship_banking : 0) );
			blit_sprite2_darken(VGAScreen,   this_player->x + 19 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 321);
			blit_sprite2_darken(VGAScreen,   this_player->x + 19 - mapX2Ofs + 30, this_player->y + 7 + shadowYDist, *shapes9ptr_, 340 + ((ship_banking < 0) ? -ship_banking : 0) );
		}
		else
			blit_sprite2x2_darken(VGAScreen, this_player->x - 5 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite);
	}
	if (background2 && superWild)
	{
		if (shipGr_ == 0)
		{
			blit_sprite2x2_darken(VGAScreen, this_player->x - 16 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite + 13);
			blit_sprite2x2_darken(VGAScreen, this_player->x + 8 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite + 51);
		}
		else if (shipGr_ == 1)
		{
			blit_sprite2x2_darken(VGAScreen, this_player->x - 4  - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 319);
			blit_sprite2_darken(VGAScreen,   this_player->x - 16 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 318);
			blit_sprite2_darken(VGAScreen,   this_player->x - 16 - mapX2Ofs + 30, this_player->y + 7 + shadowYDist, *shapes9ptr_, 337 + ((ship_banking > 0) ? -ship_banking : 0) );
			blit_sprite2_darken(VGAScreen,   this_player->x + 20 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, 321);
			blit_sprite2_darken(VGAScreen,   this_player->x + 20 - mapX2Ofs + 30, this_player->y + 7 + shadowYDist, *shapes9ptr_, 340 + ((ship_banking < 0) ? -ship_banking : 0) );
		}
		else
			blit_sprite2x2_darken(VGAScreen, this_player->x - 4 - mapX2Ofs + 30, this_player->y - 7 + shadowYDist, *shapes9ptr_, ship_sprite);
	}

	// Draw player ships
	{
		void (*spr2func)  ( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index ) = blit_sprite2;	
		void (*spr2x2func)( SDL_Surface *, int x, int y, Sprite2_array, unsigned int index ) = blit_sprite2x2;

		if (this_player->invulnerable_ticks > 0)
		{
			--this_player->invulnerable_ticks;
			spr2func = blit_sprite2_blend;
			spr2x2func = blit_sprite2x2_blend;
		}

		if (shipGr_ == 0) // Dragonwing
		{
			spr2x2func(VGAScreen, this_player->x - 17, this_player->y - 7, *shapes9ptr_, ship_sprite + 13);
			spr2x2func(VGAScreen, this_player->x + 7 , this_player->y - 7, *shapes9ptr_, ship_sprite + 51);
		}
		else if (shipGr_ == 1) // Nortship Z
		{
			spr2x2func(VGAScreen, this_player->x - 5 , this_player->y - 7, *shapes9ptr_, 319);
			spr2func(VGAScreen,   this_player->x - 17, this_player->y - 7, *shapes9ptr_, 318);
			spr2func(VGAScreen,   this_player->x - 17, this_player->y + 7, *shapes9ptr_, 337 + ((ship_banking > 0) ? -ship_banking : 0) );
			spr2func(VGAScreen,   this_player->x + 19, this_player->y - 7, *shapes9ptr_, 321);
			spr2func(VGAScreen,   this_player->x + 19, this_player->y + 7, *shapes9ptr_, 340 + ((ship_banking < 0) ? -ship_banking : 0) );
		}
		else
			spr2x2func(VGAScreen, this_player->x - 5 , this_player->y - 7, *shapes9ptr_, ship_sprite);
	}

	// NortSparks
	if (this_player->is_nortship && PL_ShotRepeat(this_player, SHOT_NORTSPARKS) && ship_banking != 0)
	{
		tempW = (ship_banking > 0) ? this_player->x - 7 : this_player->x + 9;
		b = player_shot_create(0, SHOT_NORTSPARKS, tempW + (mt_rand() % 8) - 4, this_player->y + (mt_rand() % 8) - 4, *mouseX_, *mouseY_, 671, 1);
		this_player->shot_repeat[SHOT_NORTSPARKS] = abs(ship_banking) - 1;
	}

	// Dragonwing's options (much more sticky to the ship)
	if (this_player->is_dragonwing)
	{
		if (this_player->sidekick[LEFT_SIDEKICK].style == 0)
		{
			this_player->sidekick[LEFT_SIDEKICK].x = this_player->x - 14 + ship_banking * 2;
			this_player->sidekick[LEFT_SIDEKICK].y = this_player->y;
		}

		if (this_player->sidekick[RIGHT_SIDEKICK].style == 0)
		{
			this_player->sidekick[RIGHT_SIDEKICK].x = this_player->x + 17 + ship_banking * 2;
			this_player->sidekick[RIGHT_SIDEKICK].y = this_player->y;
		}
	}

	// Player shots

	this_player->delta_x_shot_move = this_player->x - this_player->last_x_shot_move;
	this_player->delta_y_shot_move = this_player->y - this_player->last_y_shot_move;

	// PLAYER SHOT Change
	if (this_player->buttons[BUTTON_MODE] && !this_player->last_buttons[BUTTON_MODE])
	{
		this_player->shot_multi_pos[SHOT_SPECIAL] = 0;
		this_player->shot_multi_pos[SHOT_SPECIAL2] = 0;
		if (++this_player->weapon_mode > 2)
			this_player->weapon_mode = 1;
		this_player->items.special = ships[this_player->items.ship].special_weapons[this_player->weapon_mode - 1];

		JE_drawPortConfigButtons();
	}

	/* PLAYER SHOT Creation */

	/*SpecialShot*/
	JE_doSpecialShot(playerNum_, &this_player->armor, &this_player->shield);

	/*Normal Main Weapons*/
	if (!(twoPlayerLinked && this_player->is_dragonwing))
	{
		int min = 1, max = 2;

		for (temp = min - 1; temp < max; temp++)
		{
			const uint item = this_player->items.weapon[temp].id;

			if (item > 0)
			{
				if (PL_ShotRepeat(this_player, temp) && this_player->buttons[BUTTON_FIRE])
				{
					const uint item_power = this_player->items.weapon[temp].power - 1;

					b = player_shot_create(item, temp, this_player->x, this_player->y, *mouseX_, *mouseY_, weaponPort[item].normalOp[item_power], playerNum_);
				}
			}
		}
	}

	/*Super Charge Weapons*/
	if (this_player->is_dragonwing && PL_ShotRepeat(this_player, SHOT_CHARGE) && !twoPlayerLinked)
	{
		const JE_byte chargeRates[11] = {23, 21, 19, 17, 15, 13, 12, 11, 10, 9, 8};

		if (!twoPlayerLinked && chargeLevel > 0)
			blit_sprite2(VGAScreen, this_player->x + (shipGr_ == 0) + 1, this_player->y - 13, iconShapes, 77 + chargeLevel + chargeGr * 19);

		if (!chargeGrWait || !(--chargeGrWait))
		{
			chargeGr++;
			chargeGr %= 4;
			chargeGrWait = 3;
		}

		if (!chargeWait || !(--chargeWait))
		{
			if (chargeLevel < MAXCHARGE)
				++chargeLevel;

			chargeWait = chargeRates[this_player->items.weapon[FRONT_WEAPON].power - 1];
		}

		// Mode 2 Power 1: Charge Level 1
		// Mode 2 Power 2: Charge Level 2
		// Mode 2 Power 3: Charge Level 3
		// Mode 2 Power 4: Charge Level 4
		// Mode 2 Power 5: Charge Level 5

		if (this_player->buttons[BUTTON_FIRE])
		{
			if (chargeLevel > 0)
			{
				this_player->shot_multi_pos[SHOT_CHARGE] = 0;
				b = player_shot_create(16, SHOT_CHARGE,
					this_player->x, this_player->y, *mouseX_, *mouseY_,
					weaponPort[this_player->items.weapon[FRONT_WEAPON].id].chargeOp[chargeLevel - 1],
					playerNum_);
			}

			chargeLevel = 0;
			chargeWait = chargeRates[this_player->items.weapon[FRONT_WEAPON].power - 1];
		}
	}
	else
	{

	}


	// sidekicks

	if (this_player->sidekick[LEFT_SIDEKICK].style == 4 && this_player->sidekick[RIGHT_SIDEKICK].style == 4)
		this_player->satRotate += 0.2f;
	else if (this_player->sidekick[LEFT_SIDEKICK].style == 4 || this_player->sidekick[RIGHT_SIDEKICK].style == 4)
		this_player->satRotate += 0.15f;

	switch (this_player->sidekick[LEFT_SIDEKICK].style)
	{
	case 1:  // trailing
	case 3:
		this_player->sidekick[LEFT_SIDEKICK].x = this_player->old_x[COUNTOF(player->old_x) / 2 - 1];
		this_player->sidekick[LEFT_SIDEKICK].y = this_player->old_y[COUNTOF(player->old_x) / 2 - 1];
		break;
	case 2:  // front-mounted
		this_player->sidekick[LEFT_SIDEKICK].x = this_player->x;
		this_player->sidekick[LEFT_SIDEKICK].y = MAX(10, this_player->y - 20);
		break;
	case 4:  // orbitting
		this_player->sidekick[LEFT_SIDEKICK].x = this_player->x + roundf(sinf(this_player->satRotate) * 20);
		this_player->sidekick[LEFT_SIDEKICK].y = this_player->y + roundf(cosf(this_player->satRotate) * 20);
		break;
	}

	switch (this_player->sidekick[RIGHT_SIDEKICK].style)
	{
	case 4:  // orbitting
		this_player->sidekick[RIGHT_SIDEKICK].x = this_player->x - roundf(sinf(this_player->satRotate) * 20);
		this_player->sidekick[RIGHT_SIDEKICK].y = this_player->y - roundf(cosf(this_player->satRotate) * 20);
		break;
	case 1:  // trailing
	case 3:
		this_player->sidekick[RIGHT_SIDEKICK].x = this_player->old_x[0];
		this_player->sidekick[RIGHT_SIDEKICK].y = this_player->old_y[0];
		break;
	case 2:  // front-mounted
		if (!this_player->attachLinked)
		{
			this_player->sidekick[RIGHT_SIDEKICK].y += this_player->attachMove >> 1;
			if (this_player->attachMove >= -2)
			{
				if (this_player->attachReturn)
					temp = 2;
				else
					temp = 0;

				if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) + 5)
				{
					temp = 2;
					this_player->attachMove -= 1 + this_player->attachReturn;
				}
				else if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) - 0)
				{
					temp = 3;
					if (this_player->attachMove > 0)
						this_player->attachMove--;
					else
						this_player->attachMove++;
				}
				else if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) - 5)
				{
					temp = 2;
					this_player->attachMove++;
				}
				else if (this_player->attachMove < 2 + this_player->attachReturn * 4)
				{
					this_player->attachMove += 1 + this_player->attachReturn;
				}

				if (this_player->attachReturn)
					temp = temp * 2;
				if (abs(this_player->sidekick[RIGHT_SIDEKICK].x - this_player->x) < temp)
					temp = 1;

				if (this_player->sidekick[RIGHT_SIDEKICK].x > this_player->x)
					this_player->sidekick[RIGHT_SIDEKICK].x -= temp;
				else if (this_player->sidekick[RIGHT_SIDEKICK].x < this_player->x)
					this_player->sidekick[RIGHT_SIDEKICK].x += temp;

				if (abs(this_player->sidekick[RIGHT_SIDEKICK].y - (this_player->y - 20)) + abs(this_player->sidekick[RIGHT_SIDEKICK].x - this_player->x) < 8)
				{
					this_player->attachLinked = true;
					soundQueue[2] = S_CLINK;
				}

				if (this_player->buttons[BUTTON_SKICK])
					this_player->attachReturn = true;
			}
			else  // sidekick needs to catch up to player
			{
				this_player->attachMove += 1 + this_player->attachReturn;
				JE_setupExplosion(this_player->sidekick[RIGHT_SIDEKICK].x + 1, this_player->sidekick[RIGHT_SIDEKICK].y + 10, 0, 0, false, false);
			}
		}
		else
		{
			this_player->sidekick[RIGHT_SIDEKICK].x = this_player->x;
			this_player->sidekick[RIGHT_SIDEKICK].y = this_player->y - 20;
			if (this_player->buttons[BUTTON_SKICK])
			{
				this_player->attachLinked = this_player->attachReturn = false;
				this_player->attachMove = -20;
				soundQueue[3] = S_WEAPON_26;
			}
		}

		if (this_player->sidekick[RIGHT_SIDEKICK].y < 10)
			this_player->sidekick[RIGHT_SIDEKICK].y = 10;
		break;
	}

	// All players can (possibly) have sidekicks
	JE_byte last_opt_pre_loop = this_player->last_opt_fired;
	bool fired_ammo_opt = false;

	for (uint fake_i = 0; fake_i < COUNTOF(player->items.sidekick); ++fake_i)
	{
		uint i = (last_opt_pre_loop == 1) ? fake_i : 1 - fake_i;
		uint shot_i = (i == 0) ? SHOT_LEFT_SIDEKICK : SHOT_RIGHT_SIDEKICK;

		JE_OptionType *this_option = &options[this_player->items.sidekick[i]];

		// fire sidekick
		if (this_player->items.sidekick[i] > 0)
		{
			if (PL_ShotRepeat(this_player, shot_i))
			{
				if ((this_player->buttons[BUTTON_FIRE] && !this_player->sidekick[i].ammo) ||
					(this_player->buttons[BUTTON_SKICK] &&
						(!this_player->sidekick[i].ammo || (!this_player->last_buttons[BUTTON_SKICK] && !fired_ammo_opt))
					))
				{
					b = player_shot_create(this_option->wport, shot_i, this_player->sidekick[i].x, this_player->sidekick[i].y, *mouseX_, *mouseY_, this_option->wpnum + this_player->sidekick[i].charge, playerNum_);

					if (this_player->sidekick[i].charge > 0)
					{
						this_player->shot_multi_pos[shot_i] = 0;
						this_player->sidekick[i].charge = 0;
					}
					this_player->sidekick[i].charge_ticks = 20;
					this_player->sidekick[i].animation_enabled = true;

					// ammo does not refill
					// if ammo runs out, sidekick goes away
					if (this_player->sidekick[i].ammo > 0)
					{
						this_player->last_opt_fired = i;
						fired_ammo_opt = true;

						if (!(--this_player->sidekick[i].ammo))
						{
							this_player->items.sidekick[i] = 0;
							JE_updateOption(this_player, i);
						}
						else
						{
							// draw sidekick discharge ammo gauge
							const int x = hud_sidekickX[playerNum_-1], y = hud_sidekickY[i] + 14;
							fill_rectangle_xy(VGAScreenSeg, x, y, x+25, y + 2, 0);
							draw_segmented_gauge(VGAScreenSeg, x + 1, y, 112, 2, 2, MAX(1, this_player->sidekick[i].ammo_max / 8), this_player->sidekick[i].ammo);
						}
					}
				}
			}
		}
	}

	// draw sidekicks
	for (uint i = 0; i < COUNTOF(this_player->sidekick); ++i)
	{
		JE_OptionType *this_option = &options[this_player->items.sidekick[i]];

		if (this_option->option > 0)
		{
			if (this_player->sidekick[i].animation_enabled)
			{
				if (++this_player->sidekick[i].animation_frame >= this_option->ani)
				{
					this_player->sidekick[i].animation_frame = 0;
					this_player->sidekick[i].animation_enabled = (this_option->option == 1);
				}
			}

			const int x = this_player->sidekick[i].x,
			          y = this_player->sidekick[i].y;
			const uint sprite = this_option->gr[this_player->sidekick[i].animation_frame] + this_player->sidekick[i].charge;

			if (this_player->sidekick[i].style == 1 || this_player->sidekick[i].style == 2)
				blit_sprite2x2(VGAScreen, x - 6, y, iconShapes, sprite);
			else
				blit_sprite2(VGAScreen, x, y, shapes9, sprite);
		}

		if (--this_player->sidekick[i].charge_ticks == 0)
		{
			if (this_player->sidekick[i].charge < this_option->pwr)
				++this_player->sidekick[i].charge;
			this_player->sidekick[i].charge_ticks = 20;
		}
	}
}

void JE_mainGamePlayerFunctions( void )
{
	/*PLAYER MOVEMENT/MOUSE ROUTINES*/
	int cameraXFocusTarget = 160;

	if (endLevel && levelEnd > 0)
	{
		levelEnd--;
		levelEndWarp++;
	}

	/*Reset Street-Fighter commands*/
	memset(SFExecuted, 0, sizeof(SFExecuted));

	shipGr = ships[player[0].items.ship].shipgraphic;
	shipGr2 = ships[player[1].items.ship].shipgraphic;

	if (PL_NumPlayers() == 2 && player[0].is_dragonwing)
	{
		// if Player 1 is a DragonWing, handle player 2 before player 1
		// so P2's movement is updated first, drawn first, etc
		JE_playerMovement(&player[1], 2, shipGr2, shipGr2ptr, &mouseX, &mouseY);
		JE_playerMovement(&player[0], 1, shipGr,  shipGrPtr,  &mouseX, &mouseY);
	}
	else
	{
		if (player[0].player_status == STATUS_INGAME)
			JE_playerMovement(&player[0], 1, shipGr,  shipGrPtr,  &mouseX, &mouseY);
		if (player[1].player_status == STATUS_INGAME)
			JE_playerMovement(&player[1], 2, shipGr2, shipGr2ptr, &mouseX, &mouseY);
	}

	/* == Parallax Map Scrolling == */
	if (PL_NumPlayers() == 2)
		cameraXFocusTarget = (player[0].x + player[1].x) / 2;
	else if (player[0].player_status == STATUS_INGAME)
		cameraXFocusTarget = player[0].x;
	else if (player[1].player_status == STATUS_INGAME)
		cameraXFocusTarget = player[1].x;

	// Smooth out sudden jumps
	if (cameraXFocus == -1) // First gameplay frame -- jump to focus target
		cameraXFocus = cameraXFocusTarget;
	else if (abs(cameraXFocus - cameraXFocusTarget) > 8)
	{
		printf("camera playing catchup: %d to %d\n", cameraXFocus, cameraXFocusTarget);
		cameraXFocus = (cameraXFocus + cameraXFocusTarget) / 2;
	}
	else
		cameraXFocus = cameraXFocusTarget;

	tempW = floorf((260.0f - (cameraXFocus - 36.0f)) / (260.0f - 36.0f) * (24.0f * 3.0f) - 1.0f);
	mapX3Ofs   = tempW;
	mapX3Pos   = mapX3Ofs % 24;
	mapX3bpPos = 1 - (mapX3Ofs / 24);

	mapX2Ofs   = (tempW * 2) / 3;
	mapX2Pos   = mapX2Ofs % 24;
	mapX2bpPos = 1 - (mapX2Ofs / 24);

	oldMapXOfs = mapXOfs;
	mapXOfs    = mapX2Ofs / 2;
	mapXPos    = mapXOfs % 24;
	mapXbpPos  = 1 - (mapXOfs / 24);

	if (background3x1)
	{
		mapX3Ofs = mapXOfs;
		mapX3Pos = mapXPos;
		mapX3bpPos = mapXbpPos - 1;
	}
}

void JE_playerCollide( Player *this_player, JE_byte playerNum_ )
{
	// Dragonwing doesn't collide with anything when linked
	if (this_player->is_dragonwing && twoPlayerLinked)
		return;

	for (int z = 0; z < 125; z++)
	{
		if (enemyAvail[z] != 1)
		{
			int enemy_screen_x = enemy[z].ex + enemy[z].mapoffset;

			if (abs(this_player->x - enemy_screen_x) < 12 && abs(this_player->y - enemy[z].ey) < 14)
			{   /*Collide*/
				int evalue = enemy[z].evalue;

				if (evalue >= 30000)
				{
					JE_boolean awardPoints = true;

					if (evalue >= 30011 && evalue <= 30015)
					{
						// Death-dropped powerup, don't award score
						evalue -= 10;
						awardPoints = false;
					}
					if (evalue >= 30001 && evalue <= 30005)
					{
						// Powerups aren't contactable until they start falling,
						// so death-sprayed powerups don't get accidentally stolen
						if (enemy[z].eycc)
							continue;

						uint pw = evalue - 30000-1;

						this_player->shot_multi_pos[SHOT_NORMAL] = 0;
						this_player->shot_repeat[SHOT_NORMAL] = 10;

						tempW = ships[this_player->items.ship].port_weapons[pw];
						char *wName = JE_trim(weaponPort[tempW].name);
						char *color = JE_textColorFromPWeapon(pw);

						if (pw == this_player->cur_weapon)
						{
							if (PL_NumPlayers() == 2)
								sprintf(tmpBuf.s, "Player %d's ", playerNum_);
							else
								tmpBuf.s[0] = 0;

							if (PL_PowerUpWeapon(this_player, FRONT_WEAPON))
								sprintf(tmpBuf.l, "%s%s%s ^04%s", tmpBuf.s, color, wName, "powered up");
							else
							{
								if (awardPoints)
									this_player->cash += 750;
								sprintf(tmpBuf.l, "%s%s%s ^04%s", tmpBuf.s, color, wName, "power is maxed!");
							}
						}
						else
						{
							if (PL_NumPlayers() == 2)
								sprintf(tmpBuf.s, "Player %d got", playerNum_);
							else
								sprintf(tmpBuf.s, "You got");

							if (!strncmp(wName, "The", 3))
								sprintf(tmpBuf.l, "%s %s%s", tmpBuf.s, color, wName);
							else
								sprintf(tmpBuf.l, "%s the %s%s", tmpBuf.s, color, wName);
						}
						JE_drawTextWindowColorful(tmpBuf.l);
						if (awardPoints)
							this_player->cash += 250;

						this_player->cur_weapon = pw;
						this_player->items.weapon[FRONT_WEAPON].id = tempW;
						soundQueue[7] = S_POWERUP;
						enemyAvail[z] = 1;
					}
					else
					{
						// uncaught powerup
						// formerly Purple Ball / Galaga DragonWing
						soundQueue[7] = S_POWERUP;						
					}
				}
				else if (evalue > 20000)
				{
					if (twoPlayerLinked)
					{
						// share the armor evenly between linked players
						for (uint i = 0; i < COUNTOF(player); ++i)
						{
							player[i].armor += (evalue - 20000) / COUNTOF(player);
							if (player[i].armor > 28)
								player[i].armor = 28;
						}
					}
					else
					{
						this_player->armor += evalue - 20000;
						if (this_player->armor > 28)
							this_player->armor = 28;
					}
					enemyAvail[z] = 1;
					JE_drawArmor();
					soundQueue[7] = S_POWERUP;
				}
				else if (evalue > 10000 && enemyAvail[z] == 2)
				{
					if (!bonusLevel)
					{
						play_song(30);  /*Zanac*/
						bonusLevel = true;
						nextLevel = evalue - 10000;
						enemyAvail[z] = 1;
						secretLevelDisplayTime = 150;
					}
				}
				else if (enemy[z].scoreitem)
				{
					enemyAvail[z] = 1;
					soundQueue[7] = S_ITEM;
					if (evalue == 1)
					{
						cubeMax++;
						soundQueue[3] = V_DATA_CUBE;
					}
					else if (evalue == -1 || evalue == -2) // Shield powerup (formerly front/rear power)
					{
						if (PL_NumPlayers() == 2)
							sprintf(tmpBuf.s, "Player %d's s", playerNum_);
						else
							strcpy(tmpBuf.s, "S");

						if (this_player->items.shield >= SHIELD_NUM)
							sprintf(tmpBuf.l, "%shield power is maxed!", tmpBuf.s);
						else
						{
							++this_player->items.shield;
							sprintf(tmpBuf.l, "%shield powered up", tmpBuf.s);
						}
						JE_drawTextWindow(tmpBuf.l);

						this_player->shield_max = shields[this_player->items.shield].mpwr * 2;

						soundQueue[7] = S_POWERUP;

						JE_drawShield();
					}
					else if (evalue == -3)
					{
						// picked up orbiting asteroid killer
						b = player_shot_create(0, SHOT_MISC, this_player->x, this_player->y, mouseX, mouseY, 104, playerNum_);
						// This is almost certainly a bad copy-paste
						//shotAvail[z] = 0;
					}
					else if (evalue == -4)
					{
						// NOTE -- Superbomb gives a Superbomb Sidekick now
						soundQueue[7] = S_POWERUP;

						JE_byte nOption = (this_player->last_opt_given == 1) ? 0 : 1;
						if (this_player->items.sidekick[LEFT_SIDEKICK] == 0)
							nOption = 0;
						else if (this_player->items.sidekick[RIGHT_SIDEKICK] == 0)
							nOption = 1;

						this_player->items.sidekick[nOption] = 31; // Superbomb
						this_player->shot_multi_pos[SHOT_LEFT_SIDEKICK + nOption] = 0;
						this_player->shot_repeat[SHOT_LEFT_SIDEKICK + nOption] = 10;			
						JE_updateOption(this_player, nOption);
						this_player->last_opt_given = nOption;
					}
					else if (evalue == -5)
					{
						player[0].items.weapon[FRONT_WEAPON].id = 25;  // HOT DOG!
						player[0].items.weapon[REAR_WEAPON].id = 26;
						player[1].items.weapon[REAR_WEAPON].id = 26;

						player[0].last_items = player[0].items;

						for (uint i = 0; i < COUNTOF(player); ++i)
							player[i].weapon_mode = 1;

						player[0].shot_multi_pos[FRONT_WEAPON] = 0;
						player[0].shot_multi_pos[REAR_WEAPON] = 0;
						player[1].shot_multi_pos[REAR_WEAPON] = 0;
					}
					else if (twoPlayerLinked)
					{
						// players get equal share of pick-up cash when linked
						for (uint i = 0; i < COUNTOF(player); ++i)
							player[i].cash += evalue / COUNTOF(player);
					}
					else
					{
						this_player->cash += evalue;
					}
					JE_setupExplosion(enemy_screen_x, enemy[z].ey, 0, enemyDat[enemy[z].enemytype].explosiontype, true, false);
				}
				else if (this_player->invulnerable_ticks == 0 && enemyAvail[z] == 0 &&
				         (enemyDat[enemy[z].enemytype].explosiontype & 1) == 0) // explosiontype & 1 == 0: not ground enemy
				{
					int armorleft = enemy[z].armorleft;
					if (armorleft > damageRate)
						armorleft = damageRate;

					PL_PlayerDamage(this_player, armorleft);

					// player ship gets push-back from collision
					if (enemy[z].armorleft > 0)
					{
						this_player->x_velocity += (enemy[z].exc * enemy[z].armorleft) / 2;
						this_player->y_velocity += (enemy[z].eyc * enemy[z].armorleft) / 2;
					}

					int armorleft2 = enemy[z].armorleft;
					if (armorleft2 == 255)
						armorleft2 = 30000;

					temp = enemy[z].linknum;
					if (temp == 0)
						temp = 255;

					b = z;

					if (armorleft2 > armorleft)
					{
						// damage enemy
						if (enemy[z].armorleft != 255)
							enemy[z].armorleft -= armorleft;
						soundQueue[5] = S_ENEMY_HIT;
					}
					else
					{
						// kill enemy
						for (temp2 = 0; temp2 < 100; temp2++)
						{
							if (enemyAvail[temp2] != 1)
							{
								temp3 = enemy[temp2].linknum;
								if (temp2 == b ||
									(temp != 255 &&
									 (temp == temp3 || temp - 100 == temp3
									  || (temp3 > 40 && temp3 / 20 == temp / 20 && temp3 <= temp))))
								{
									int enemy_screen_x = enemy[temp2].ex + enemy[temp2].mapoffset;

									enemy[temp2].linknum = 0;

									enemyAvail[temp2] = 1;

									if (enemyDat[enemy[temp2].enemytype].esize == 1)
									{
										JE_setupExplosionLarge(enemy[temp2].enemyground, enemy[temp2].explonum, enemy_screen_x, enemy[temp2].ey);
										soundQueue[6] = S_EXPLOSION_9;
									}
									else
									{
										JE_setupExplosion(enemy_screen_x, enemy[temp2].ey, 0, 1, false, false);
										soundQueue[5] = S_EXPLOSION_4;
									}
								}
							}
						}
						enemyAvail[z] = 1;
					}
				}
			}

		}
	}
}

