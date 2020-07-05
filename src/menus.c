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
#include "font.h"
#include "fonthand.h"
#include "input.h"
#include "mainint.h"
#include "menus.h"
#include "nortsong.h"
#include "opentyr.h"
#include "palette.h"
#include "pcxload.h"
#include "picload.h"
#include "player.h"
#include "sprite.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

char episode_name[6][31];

static const JE_shortint shipXpos[] = {
	 0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 
	30, 32, 34, 36, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22,
	20, 18, 16, 14, 12, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,
};
static const JE_byte pcolor[] = {96+48, 96+16};

static inline void outTextMirrorX( SDL_Surface * screen, int x, int y, const char *s, unsigned int filter, int brightness, unsigned int font, JE_boolean shadow, JE_byte p )
{
	JE_outTextAdjust(screen, 
		(p == 0) ? x : (320 - x) - JE_textWidth(s, SMALL_FONT_SHAPES), y, 
		s, filter, brightness, font, shadow);
}

static void draw_shipGraphic( int x, int y, uint sGr, int facing )
{
	if (sGr == 0) // Dragonwing
	{
		blit_sprite2x2(VGAScreen, x - 24, y, shapes9, 13 + (facing * 2));
		blit_sprite2x2(VGAScreen, x     , y, shapes9, 51 + (facing * 2));
	}
	else if (sGr == 1) // Nortship
	{
		blit_sprite2x2(VGAScreen, x - 12, y,      shapes9, 319);
		blit_sprite2(VGAScreen,   x - 24, y,      shapes9, 318);
		blit_sprite2(VGAScreen,   x - 24, y + 14, shapes9, 337 + ((facing > 0) ? -facing : 0) );
		blit_sprite2(VGAScreen,   x + 12, y,      shapes9, 321);
		blit_sprite2(VGAScreen,   x + 12, y + 14, shapes9, 340 + ((facing < 0) ? -facing : 0) );
	}
	else
		blit_sprite2x2(VGAScreen, x - 12, y, shapes9, sGr + (facing * 2));
}

static void draw_PlayerStatusText( JE_byte p )
{
	JE_word cF = 1, cP = 0xFFFF;
	if (player[p].player_status == STATUS_SELECT)
	{
		outTextMirrorX(VGAScreen, 12, 136, "Wait For", 15, -4, SMALL_FONT_SHAPES, true, p);
		outTextMirrorX(VGAScreen, 12, 152, "Other Player", 15, -4, SMALL_FONT_SHAPES, true, p);
		return;
	}

	ARC_GetCredits(&cF, &cP, NULL);
	if (!cF)
		strcpy(tmpBuf.s, "Insert Coin");
	else
		strcpy(tmpBuf.s, "Press Fire");

	outTextMirrorX(VGAScreen, 12, 136, tmpBuf.s, 15, -4, SMALL_FONT_SHAPES, true, p);

	if (cP == 0xFFFF)
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Free Play");
	else if (!cP)
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu", cF);
	else
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu %hu/%hu", cF, cP, (JE_word)DIP.coinsPerGame);

	outTextMirrorX(VGAScreen, 12, 152, tmpBuf.s, 15, -4, SMALL_FONT_SHAPES, true, p);
}

void select_gameplay( void )
{
	bool fade_in = true;

	// Nortship secret unlock
	JE_byte max_ship_select = SHIPORDER_NOSECRET, rightloops = 7;
	JE_byte ship_select[2] = {0, 1};
	bool twoP = (player[0].player_status == player[1].player_status);

	size_t shipXofs = 1, lastXofs = 0;
	int shipAngle;

	uint nTimer, tTimer = SDL_GetTicks() + 20999;

	JE_loadPCX(arcdata_dir(), "select.pcx");
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	for (; ; )
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
		JE_dString(VGAScreen, JE_fontCenter("Select Ship", FONT_SHAPES), 20, "Select Ship", FONT_SHAPES);

		if (mainLevel != FIRST_LEVEL)
		{
			sprintf(tmpBuf.l, "DEBUG: Starting at ID %hu", mainLevel);
			JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 36, tmpBuf.l, 15, 3, FULL_SHADE);
		}

		lastXofs = shipXofs;
		if (++shipXofs >= sizeof(shipXpos))
			shipXofs = 0;
		shipAngle = shipXpos[shipXofs] - shipXpos[lastXofs];
		

		for (int p = 0; p < 2; ++p)
		{
			if (player[p].player_status != STATUS_SELECT && player[p].player_status != STATUS_INGAME)
			{
				draw_PlayerStatusText(p);
				continue;
			}

			int x = -20 + 35 * (ship_select[p] + 1);
			int y = 50 + (ship_select[p] & 1) * 40;

			strcpy(tmpBuf.l, JE_trim(ships[shiporder[ship_select[p]]].name));

			if (ship_select[p] >= SHIPORDER_NOSECRET) // On the Nortship
			{
				fill_rectangle_xy(VGAScreen,   0, y,  19, 90, pcolor[p] +5);
				fill_rectangle_xy(VGAScreen, 300, y, 319, 90, pcolor[p] +5);
			}
			else
				fill_rectangle_xy(VGAScreen, x, y, x + 39, y + 40, pcolor[p] +5);
			if (p == 0)
				JE_textShade(VGAScreen, x + 1, y + 1, "1P", pcolor[p]/16, 6, FULL_SHADE);
			else
				JE_textShade(VGAScreen, x + 30, y + 34, "2P", pcolor[p]/16, 6, FULL_SHADE);

			outTextMirrorX(VGAScreen, 12, 136, tmpBuf.l, 15, -4, SMALL_FONT_SHAPES, true, p);

			if (p == 0)
				draw_shipGraphic(50  + shipXpos[shipXofs], 160, ships[shiporder[ship_select[p]]].shipgraphic, shipAngle);
			else
				draw_shipGraphic(270 - shipXpos[shipXofs], 160, ships[shiporder[ship_select[p]]].shipgraphic, -shipAngle);

			if (player[p].player_status == STATUS_INGAME)
				outTextMirrorX(VGAScreen, 136, 182, "OK", 15, -4, SMALL_FONT_SHAPES, true, p);
		}
		for (int i = 1; i <= SHIPORDER_NOSECRET; ++i)
		{
			int x = 35 * i;
			int y = 54 + ((i & 1) ? 0 : 40);
			draw_shipGraphic(x, y, ships[shiporder[i - 1]].shipgraphic, 0);
		}

		nTimer = tTimer - SDL_GetTicks();
		if (nTimer > 21000)
			nTimer = 0;

		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%d", nTimer / 1000);
		JE_outTextAdjust(VGAScreen, 
			JE_fontCenter(tmpBuf.s, SMALL_FONT_SHAPES), 136, 
			tmpBuf.s, 15, -4, 
			SMALL_FONT_SHAPES, true);

		if (fade_in)
		{
			fade_palette(colors, 10, 0, 255);
			fade_in = false;
		}

		++arcTextTimer;
		arcTextTimer %= 200;

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		// Timer expired?
		if (!nTimer)
		{
			for (int i = 0; i < 2; ++i)
			{
				if (player[i].player_status == STATUS_SELECT)
					player[i].player_status = STATUS_INGAME;
			}
			JE_playSampleNum(S_SELECT);
		}

		uint button = 0;
		while (I_inputForMenu(&button, INPUT_P2_FIRE))
		{
			uint p = 0;
			switch (button++)
			{
			case INPUT_P1_UP:
			case INPUT_P2_UP:
				mainLevel--;
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_DOWN:
			case INPUT_P2_DOWN:
				mainLevel++;
				JE_playSampleNum(S_CURSOR);
				break;

			case INPUT_P2_FIRE:
				p = 1;
				// fall through
			case INPUT_P1_FIRE:
				if (player[p].player_status == STATUS_SELECT)
				{
					JE_playSampleNum(S_SELECT);
					player[p].player_status = STATUS_INGAME;
					break;
				}
				else if (player[p].player_status != STATUS_INGAME && ARC_CoinStart())
				{
					JE_playSampleNum(S_SELECT);
					player[p].player_status = STATUS_SELECT;
					twoP = true;
					if (ship_select[0] == ship_select[1])
						ship_select[p]++;
				}
				break;

			case INPUT_P2_LEFT:
				p = 1;
				// fall through
			case INPUT_P1_LEFT:
				if (player[p].player_status != STATUS_SELECT)
					break;

				JE_playSampleNum(S_CURSOR);
				rightloops = 7; // Nortship locking
				do
				{
					if (--ship_select[p] == 255)
						ship_select[p] = max_ship_select - 1;
				} while (twoP && ship_select[0] == ship_select[1]);
				break;
			case INPUT_P2_RIGHT:
				p = 1;
				// fall through
			case INPUT_P1_RIGHT:
				if (player[p].player_status != STATUS_SELECT)
					break;

				JE_playSampleNum(S_CURSOR);
				do
				{
					if (++ship_select[p] >= max_ship_select)
					{
						if (max_ship_select == SHIPORDER_NOSECRET && !(--rightloops))
						{
							JE_playSampleNumOnChannel(S_POWERUP, 1);
							max_ship_select = SHIPORDER_MAX;
						}
						else
							ship_select[p] = 0;
					}
				} while (twoP && ship_select[0] == ship_select[1]);
				break;

			default:
				break;
			}
		}

		// All players ingame are ready?
		if (player[0].player_status != STATUS_SELECT && player[1].player_status != STATUS_SELECT)
		{
			fade_black(10);

			onePlayerAction = true;
			for (int i = 0; i < 2; ++i)
			{
				if (player[i].player_status == STATUS_INGAME)
					player[i].items.ship = shiporder[ship_select[i]];
			}
			return;
		}
	}
}

void select_episode( void )
{
	bool selection_made = false;
	JE_byte in_control = (player[0].player_status == STATUS_SELECT) ? 1 : 2;
	uint nTimer, tTimer = SDL_GetTicks() + 20999;

	// In case music was stopped.
	play_song(SONG_TITLE);

	JE_loadPCX(arcdata_dir(), "select.pcx");
	JE_dString(VGAScreen, JE_fontCenter(episode_name[0], FONT_SHAPES), 20, episode_name[0], FONT_SHAPES);

	int episode = 1, episode_max = EPISODE_AVAILABLE;

	if (!episodeAvail[4]) // If episode 5 isn't available, don't show it.
		--episode_max;

	bool fade_in = true;

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	for (; ; )
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (int p = 0; p < 2; ++p)
		{
			if (player[p].player_status != STATUS_SELECT || (p + 1) != in_control)
			{
				draw_PlayerStatusText(p);
				continue;
			}
		}

		int y = 44;
		for (int i = 1; i <= episode_max; i++)
		{
			JE_outTextAdjust(VGAScreen, 20, y, episode_name[i], 15, -4 + (i == episode ? 2 : 0) - (!episodeAvail[i - 1] ? 4 : 0), SMALL_FONT_SHAPES, true);
			y += (episode_max == 5) ? 18 : 24;
		}

		nTimer = tTimer - SDL_GetTicks();
		if (nTimer > 21000)
			nTimer = 0;

		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%d", nTimer / 1000);
		JE_outTextAdjust(VGAScreen, 
			JE_fontCenter(tmpBuf.s, SMALL_FONT_SHAPES), 136, 
			tmpBuf.s, 15, -4, 
			SMALL_FONT_SHAPES, true);

		if (fade_in)
		{
			fade_palette(colors, 10, 0, 255);
			fade_in = false;
		}

		++arcTextTimer;
		arcTextTimer %= 200;

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		// Timer expired?
		if (!nTimer)
			selection_made = true;

		uint button = 0;
		while (I_inputForMenu(&button, INPUT_P2_FIRE))
		{
			switch (button++)
			{
			case INPUT_P1_FIRE:
				if (player[0].player_status != STATUS_SELECT && ARC_CoinStart())
				{
					JE_playSampleNum(S_SELECT);
					player[0].player_status = STATUS_SELECT;
				}
				else if (in_control == 1)
					selection_made = true;
				break;
			case INPUT_P2_FIRE:
				if (player[1].player_status != STATUS_SELECT && ARC_CoinStart())
				{
					JE_playSampleNum(S_SELECT);
					player[1].player_status = STATUS_SELECT;
				}
				else if (in_control == 2)
					selection_made = true;
				break;

			case INPUT_P1_UP:
				if (in_control == 2)
					break;
				if (true) {} else // yes, this madness does actually work
				// fall through
			case INPUT_P2_UP:
				if (in_control == 1)
					break;

				do
				{
					if (--episode < 1)
						episode = episode_max;
				} while (!episodeAvail[episode - 1]);
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_DOWN:
				if (in_control == 2)
					break;
				if (true) {} else // yes, this madness does actually work
				// fall through
			case INPUT_P2_DOWN:
				if (in_control == 1)
					break;

				do
				{
					if (++episode > episode_max)
						episode = 1;
				} while (!episodeAvail[episode - 1]);
				JE_playSampleNum(S_CURSOR);
				break;

			default:
				break;
			}
		}

		if (selection_made)
		{
			if (!episodeAvail[episode - 1])
				episode = 1;
			JE_playSampleNum(S_SELECT);
			fade_black(10);

			JE_initEpisode(episode);
			initial_episode_num = episodeNum;
			return;
		}
	}
}

void JE_titleScreen( void )
{
	uint button;
	int tyrY = 62, t2kY = 41;

	//const int menunum = 7;
	skip_header_draw = false;
	skip_header_undraw = false;

	//unsigned int arcade_code_i[SA_ENGAGE] = { 0 };

	JE_word waitForDemo;
	JE_word oldCoins;

	//JE_byte menu = 0;
	//JE_boolean redraw = true,
	//           fadeIn = false;

	play_demo = false;

	//redraw = true;
	bool fadeIn = true;

	gameLoaded = false;
	jumpSection = false;

	set_volume(tyrMusicVolume, fxVolume);

	// ARCADE TITLE SCREEN STARTUP
	play_song_once(SONG_TITLE);
	arcTextTimer = 0;

	JE_loadPic(VGAScreen, 4, false);

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	blit_sprite(VGAScreenSeg, 11, tyrY, PLANET_SHAPES, 146); // tyrian logo
	if (tyrian2000detected)
		blit_sprite(VGAScreenSeg, 155, t2kY, PLANET_SHAPES, 151); // 2000(tm)

	JE_showVGA();

	fade_palette(colors, 10, 0, 255 - 16);

	if (attractTic == 0)
		attractTic = SDL_GetTicks();

	if (true)
	{
		tyrY = 61;
		t2kY = 44;
		for (int i = 0; i < 29; ++i)
		{
			setjasondelay(2);

			memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

			blit_sprite(VGAScreenSeg, 11, tyrY, PLANET_SHAPES, 146); // tyrian logo
			if (tyrian2000detected)
				blit_sprite(VGAScreenSeg, 155, t2kY, PLANET_SHAPES, 151); // 2000(tm)

			JE_showVGA();
			service_wait_delay();

			tyrY -= 2;
			++t2kY;
		}
	}

	oldCoins = ARC_GetCoins();
	waitForDemo = (oldCoins >= DIP.coinsPerGame) ? 35*10 : 35*5;

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);
	do
	{
		setjasondelay(2);
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen2->pitch * VGAScreen2->h);

		int x = VGAScreen->w / 2, y = 110;
		JE_word cF = 1, cP = 0xFFFF;

		if (tyrian2000detected)
			y += 32;

		ARC_GetCredits(&cF, &cP, NULL);
		if (!cF)
			strcpy(tmpBuf.s, "Insert Coin");
		else
			strcpy(tmpBuf.s, "Press Fire");
		draw_font_hv(VGAScreen, x - 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x - 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x,     y,     tmpBuf.s, normal_font, centered, 15, -3);
		y += 16;

		if (cP == 0xFFFF)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Free Play");
		else if (!cP)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu", cF);
		else
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu %hu/%hu", cF, cP, (JE_word)DIP.coinsPerGame);
		draw_font_hv(VGAScreen, x - 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x - 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x,     y,     tmpBuf.s, normal_font, centered, 15, -3);

		if (fadeIn)
		{
			fade_palette(colors, 20, 255 - 16 + 1, 255); // fade in menu items
			fadeIn = false;
		}

		++arcTextTimer;
		arcTextTimer %= 200;

		JE_showVGA();
		wait_delay();

		I_checkButtons();
		button = INPUT_P1_FIRE;
		while (I_inputForMenu(&button, INPUT_P2_FIRE))
		{
			switch (button++)
			{
			case INPUT_P2_FIRE:
				if (ARC_CoinStart())
					player[1].player_status = STATUS_SELECT;
				break;
			case INPUT_P1_FIRE:
				if (ARC_CoinStart())
					player[0].player_status = STATUS_SELECT;
				break;
			}
		}

		if (player[0].player_status == STATUS_SELECT || player[1].player_status == STATUS_SELECT)
		{
			JE_playSampleNum(S_SELECT);
			fade_black(10);
			select_episode();
			select_gameplay();

			currentRank = 0;
			difficultyLevel = DIP.startingDifficulty;
			// Start special mode!
			onePlayerAction = true;
			gameLoaded = true;
			initialDifficulty = difficultyLevel;

			for (int pNum = 0; pNum < 2; ++pNum)
			{
				if (player[pNum].player_status == STATUS_INGAME)
					PL_Init(&player[pNum], player[pNum].items.ship, false);
			}
		}

		if (oldCoins != ARC_GetCoins())
		{
			oldCoins = ARC_GetCoins();
			waitForDemo = (oldCoins >= DIP.coinsPerGame) ? 35*10 : 35*5;
		}
		else
		{
			if (--waitForDemo == 0)
			{
				play_demo = true;	
			}
		}
	}
	while (!(gameLoaded || jumpSection || play_demo));

	fade_black(15);
}
