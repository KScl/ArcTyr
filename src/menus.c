/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  menus.c
/// \brief Selection menus, etc.

#include "arcade.h"
#include "arcade/service.h"

#include "config.h"
#include "episodes.h"
#include "file.h"
#include "font.h"
#include "fonthand.h"
#include "helptext.h"
#include "input.h"
#include "mainint.h"
#include "menus.h"
#include "nortsong.h"
#include "nortvars.h"
#include "opentyr.h"
#include "palette.h"
#include "pcxload.h"
#include "picload.h"
#include "playdata.h"
#include "player.h"
#include "sprite.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

char episode_name[6][31];


//
// Standard menu stuff (timer, etc.)
//

static Uint64 _endTime = 0;
static bool _outOfTime = true;
static bool _fadeIn = true;

static void _enterMenuBehaviors( int seconds )
{
	_endTime = (seconds * 1000) + 999;
	_endTime += SDL_GetTicks();

	_outOfTime = false;
	_fadeIn = true;
}

static void _regularMenuBehaviors( void )
{
	// Check and draw timer
	Uint64 t = SDL_GetTicks();
	if (t > _endTime)
	{
		t = 0;
		_outOfTime = true;
	}
	else
		t = _endTime - t;

	snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%lu", t / 1000);
	JE_outTextAdjust(VGAScreen, JE_fontCenter(tmpBuf.s, SMALL_FONT_SHAPES),
		136, tmpBuf.s, 15, -4, SMALL_FONT_SHAPES, true);

	// Fade in
	if (_fadeIn)
	{
		fade_palette(colors, 10, 0, 255);
		_fadeIn = false;
	}

	// Progress arc timer (might be unnecessary now?)
	++arcTextTimer;
	arcTextTimer %= 200;
}

static inline void _menuMirrorText( SDL_Surface * screen, int x, int y, int p, const char *s )
{
	if (p == 0)
		draw_font_hv_shadow(screen, x,       y, s, normal_font, left_aligned,  15, -4, false, 2);
	else
		draw_font_hv_shadow(screen, 320 - x, y, s, normal_font, right_aligned, 15, -4, false, 2);
}

static void _drawShipGraphic( int x, int y, JE_byte shipnum, int facing )
{
	if (shipnum == 0)
		return; // Empty slot
	if (shipnum == 0xFF)
	{
		draw_font_hv_shadow(VGAScreen, x, y + 7, "?", normal_font, centered,  15, -2, false, 2);
		return; // TODO random slot
	}

	uint sGr = ships[shipnum].shipgraphic;
	if (sGr == 0) // Dragonwing
	{
		blit_sprite2x2(VGAScreen, x - 24, y, shipShapes, 13 + (facing * 2));
		blit_sprite2x2(VGAScreen, x     , y, shipShapes, 51 + (facing * 2));
	}
	else if (sGr == 1) // Nortship
	{
		blit_sprite2x2(VGAScreen, x - 12, y,      shipShapes, 319);
		blit_sprite2(VGAScreen,   x - 24, y,      shipShapes, 318);
		blit_sprite2(VGAScreen,   x - 24, y + 14, shipShapes, 337 + ((facing > 0) ? -facing : 0) );
		blit_sprite2(VGAScreen,   x + 12, y,      shipShapes, 321);
		blit_sprite2(VGAScreen,   x + 12, y + 14, shipShapes, 340 + ((facing < 0) ? -facing : 0) );
	}
	else if (sGr >= 1000)
		blit_sprite2x2(VGAScreen, x - 12, y, shipShapesT2K, sGr + (facing * 2) - 1000);		
	else
		blit_sprite2x2(VGAScreen, x - 12, y, shipShapes, sGr + (facing * 2));
}

static void _drawPlayerStatusText( JE_byte p )
{
	if (player[p].player_status == STATUS_INGAME)
	{
		strcpy(tmpBuf.l, JE_trim(ships[player[p].items.ship].name));
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%lu", player[p].cash);
		_drawShipGraphic((p == 1) ? 270 : 50, 160, player[p].items.ship, 0);
		_menuMirrorText(VGAScreen, 12, 136, p, tmpBuf.l);
		_menuMirrorText(VGAScreen, 12, 152, p, tmpBuf.s);
		return;
	}
	if (player[p].player_status == STATUS_SELECT)
	{
		_menuMirrorText(VGAScreen, 12, 136, p, "Wait For");
		_menuMirrorText(VGAScreen, 12, 152, p, "Other Player");
		return;
	}

	JE_word curCoins = ARC_GetCoins();
	strcpy(tmpBuf.s, (curCoins >= DIP.coinsToStart) ? "Press Fire" : "Insert Coin");
	_menuMirrorText(VGAScreen, 12, 136, p, tmpBuf.s);

	if (DIP.coinsToStart == 0)
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Free Play");
	else if (DIP.coinsToStart == 1)
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu", curCoins);
	else
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu/%hu", curCoins, DIP.coinsToStart);

	_menuMirrorText(VGAScreen, 12, 152, p, tmpBuf.s);
}


//
// Ship selection
//

static const JE_shortint _timeShipX[] = {
	 0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
	30, 32, 34, 36, 38, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50, 50, 50, 50, 50, 50, 50,
	50, 50, 50, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22,
	20, 18, 16, 14, 12, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,
};
static const JE_byte _playerHue[2]   = {9,      7     };
static const JE_byte _playerColor[2] = {9 << 4, 7 << 4};

// Only referenced from title screen and new episode screen
static void Menu_selectShip( void )
{
	bool twoP = (player[0].player_status > STATUS_NONE && player[1].player_status > STATUS_NONE);
	bool selectionMade[2] = {false, false};

	const int xIncrease[2] = {300 / (num_ship_select[0]), 300 / (num_ship_select[1])};
	JE_byte ss_x[2] = {1, 4};
	JE_byte ss_y[2] = {0, 0};

	// Secret code input
	JE_byte code_length;
	JE_byte *tmp_code_buf;

	// Ship display
	size_t shipXofs = 1, lastXofs = 0;
	int shipAngle;
	JE_byte random_appearance[3] = {1, 1}; // currently displayed random ship (not used for selection)

	JE_loadPCX(arcdata_dir(), "select.pcx");
	JE_dString(VGAScreen, JE_fontCenter("Select Ship", FONT_SHAPES), 16, "Select Ship", FONT_SHAPES);
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	// If entering from mid-game (next episode screen), set cursor appropriately
	// If not, set player ship appropriately
	for (int i = 0; i < 2; ++i)
	{
		if (player[i].player_status == STATUS_INGAME)
		{
			selectionMade[i] = true;
			if (ships[player[i].items.ship].locationinmenu.present)
			{	
				ss_x[i] = ships[player[i].items.ship].locationinmenu.x;
				ss_y[i] = ships[player[i].items.ship].locationinmenu.y;
			}
			else
			{
				ss_x[i] = (i == 0) ? 0 : ships[0].locationinmenu.x;
				ss_y[i] = ships[0].locationinmenu.y;
			}
		}
		else
		{
			// Enable secret codes
			I_initCodeInput(i + 1);

#ifdef ENABLE_DEVTOOLS
			// Start on random with input fuzzing
			if (inputFuzzing)
			{
				ss_x[i] = (i == 0) ? 0 : ships[0].locationinmenu.x;
				ss_y[i] = ships[0].locationinmenu.y;
			}
#endif

			player[i].items.ship = ship_select[ss_y[i]][ss_x[i]];

			// In case P1 selected the ship P2's cursor starts on, or vice versa,
			// and then we return to this screen later
			if (player[0].items.ship == player[1].items.ship)
				player[i].items.ship = ship_select[(ss_y[i] = 1)][ss_x[i]];
		}
	}

	_enterMenuBehaviors(20);
	for (; ; )
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		lastXofs = shipXofs;
		if (++shipXofs >= sizeof(_timeShipX))
			shipXofs = 0;
		shipAngle = _timeShipX[shipXofs] - _timeShipX[lastXofs];

		for (int p = 0; p < 2; ++p)
		{
			if (player[p].player_status != STATUS_SELECT && player[p].player_status != STATUS_INGAME)
			{
				_drawPlayerStatusText(p);
				continue;
			}

			int x = 10 + (ss_x[p] * xIncrease[0]);
			int y = 44 + (ss_y[p] * 40);
			uint ship_id = player[p].items.ship;

			if (player[p].items.ship == 0xFF)
			{
				strcpy(tmpBuf.l, "Random Select?");
				ship_id = random_appearance[p];
			}
			else
				strcpy(tmpBuf.l, JE_trim(ships[ship_id].name));

			fill_rectangle_xy(VGAScreen, x, y, x + (xIncrease[0] - 1), y + 39, _playerColor[p] +5);
			if (p == 0)
			{
				JE_textShade(VGAScreen, x + 1, y + 1, "1P", _playerHue[p], 6, FULL_SHADE);
				_drawShipGraphic(50  + _timeShipX[shipXofs], 160, ship_id, shipAngle);
			}
			else
			{
				JE_textShade(VGAScreen, x + (xIncrease[0] - 10), y + 34, "2P", _playerHue[p], 6, FULL_SHADE);
				_drawShipGraphic(270 - _timeShipX[shipXofs], 160, ship_id, -shipAngle);
			}

			_menuMirrorText(VGAScreen, 12, 136, p, tmpBuf.l);
			if (player[p].player_status == STATUS_INGAME)
				_menuMirrorText(VGAScreen, 136, 182, p, "OK");
		}

		// Draw both rows of ships
		for (int section = SHIP_SELECT_TOP; section <= SHIP_SELECT_BOTTOM; ++section)
		{
			int x = 10 + (xIncrease[section] / 2);
			int y = 50 + (section * 40);
			for (int i = 0; i < num_ship_select[section]; ++i)
			{
				uint ship_id = ship_select[section][i];
				if (ship_id == 0xFF) // If random, don't display random if player's ship *isn't* random
				{
					if (ss_x[0] == i && ss_y[0] == section && player[0].items.ship != 0xFF)
						ship_id = player[0].items.ship;
					else if (ss_x[1] == i && ss_y[1] == section && player[1].items.ship != 0xFF)
						ship_id = player[1].items.ship;
				}
				_drawShipGraphic(x, y, ship_id, 0);
				x += xIncrease[section];
			}
		}

		_regularMenuBehaviors();

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		// Various always-execute player actions
		for (int i = 0; i < 2; ++i)
		{
			if (player[i].player_status != STATUS_SELECT)
				continue;

			if (_outOfTime) // Timer expired?
			{
				selectionMade[i] = true;
				continue;
			}

			// Reroll random ships
			if ((shipXofs & 3) == 3)
			{
				JE_byte old_rand = random_appearance[i];
				do
					random_appearance[i] = (mt_rand() % num_ships) + 1;
				while (random_appearance[i] == old_rand
					|| (!tyrian2000detected && ships[random_appearance[i]].shipgraphic > 1000));
			}

			tmp_code_buf = I_checkForCodeInput(i + 1, &code_length);
			if (player[i].items.ship == 0xFF && tmp_code_buf != NULL)
			{
				for (int code_i = 0; code_i < num_secret_ship_codes; ++code_i)
				{
					if (player[(i == 1) ? 0 : 1].items.ship == secret_ship_codes[code_i].ship)
						continue; // You can't BOTH use the same secret ship...
					if (!memcmp(tmp_code_buf, secret_ship_codes[code_i].code, sizeof(JE_byte) * 16))
					{
						player[i].items.ship = secret_ship_codes[code_i].ship;
						JE_playSampleNumOnChannel(S_POWERUP, 1);
						break;
					}
				}
			}
		}

		uint button = 0;
		while (I_inputForMenu(&button, INPUT_P2_FIRE))
		{
			uint p = (button >= INPUT_P2_UP) ? 1 : 0;
			switch (button++)
			{
			case INPUT_P2_FIRE:
			case INPUT_P1_FIRE:
				if (player[p].player_status == STATUS_SELECT)
					selectionMade[p] = true;
				else if (player[p].player_status != STATUS_INGAME && ARC_CoinStart(&player[p]))
				{
					JE_playSampleNum(S_SELECT);
					twoP = true;
					// If the other player's over the ship we'd start on, then move the cursor to the other row
					if (ss_x[0] == ss_x[1] && ss_y[0] == ss_y[1])
						ss_y[p] = (ss_y[p] == 1) ? 0 : 1;
					player[p].player_status = STATUS_SELECT;
					player[p].items.ship = ship_select[ss_y[p]][ss_x[p]];
				}
				break;

			case INPUT_P2_LEFT:
			case INPUT_P1_LEFT:
				if (player[p].player_status != STATUS_SELECT)
					break;
				do
				{
					if (--ss_x[p] == 255)
						ss_x[p] = num_ship_select[ss_y[p]] - 1;
				} while (ship_select[ss_y[p]][ss_x[p]] == 0 || (twoP && ss_y[0] == ss_y[1] && ss_x[0] == ss_x[1]));
				player[p].items.ship = ship_select[ss_y[p]][ss_x[p]];
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P2_RIGHT:
			case INPUT_P1_RIGHT:
				if (player[p].player_status != STATUS_SELECT)
					break;
				do
				{
					if (++ss_x[p] >= num_ship_select[ss_y[p]])
						ss_x[p] = 0;
				} while (ship_select[ss_y[p]][ss_x[p]] == 0 || (twoP && ss_y[0] == ss_y[1] && ss_x[0] == ss_x[1]));
				player[p].items.ship = ship_select[ss_y[p]][ss_x[p]];
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P2_UP:
			case INPUT_P2_DOWN:
			case INPUT_P1_UP:
			case INPUT_P1_DOWN:
				if (player[p].player_status != STATUS_SELECT)
					break;
				do
					ss_y[p] = (ss_y[p] == 1) ? 0 : 1;
				while (ship_select[ss_y[p]][ss_x[p]] == 0 || (twoP && ss_y[0] == ss_y[1] && ss_x[0] == ss_x[1]));
				player[p].items.ship = ship_select[ss_y[p]][ss_x[p]];
				JE_playSampleNum(S_CURSOR);
				break;

			default:
				break;
			}
		}

		// Behavior for selecting a ship
		// Moved down here to keep everything together
		for (int i = 0; i < 2; ++i)
		{
			if (selectionMade[i] && player[i].player_status != STATUS_INGAME)
			{
				// Selected random: Move the cursor into the proper location if possible
				if (player[i].items.ship == 0xFF)
				{
					PL_RandomSelect(&player[i]);
					if (ships[player[i].items.ship].locationinmenu.present)
					{
						ss_x[i] = ships[player[i].items.ship].locationinmenu.x;
						ss_y[i] = ships[player[i].items.ship].locationinmenu.y;
					}
				}
				// Init player data
				PL_Init(&player[i], player[i].items.ship, false); // sets STATUS_INGAME
				JE_playSampleNum(S_SELECT);
			}
		}

		// All players ingame are ready?
		if (player[0].player_status != STATUS_SELECT && player[1].player_status != STATUS_SELECT)
			break;
	}
	fade_black(10);
}

//
// Episode related menus
//

// Only referenced from title screen
static void Menu_selectEpisode( void )
{
	bool selection_made = false;
	JE_byte in_control = (player[0].player_status == STATUS_SELECT) ? 0 : 1;

	int episode = 1, episode_max = EPISODE_AVAILABLE;

	if (!episodeAvail[4]) // If episode 5 isn't available, don't show it.
		--episode_max;

	mainLevel = FIRST_LEVEL;

	JE_loadPCX(arcdata_dir(), "select.pcx");
	JE_dString(VGAScreen, JE_fontCenter(episode_name[0], FONT_SHAPES), 16, episode_name[0], FONT_SHAPES);
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	// In case the music was stopped, this will restart it.
	play_song(SONG_TITLE);

	_enterMenuBehaviors(20);
	while (!selection_made)
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (int i = 0; i < 2; ++i)
		{
			if (player[i].player_status != STATUS_SELECT || i != in_control)
			{
				_drawPlayerStatusText(i);
				continue;
			}
		}

		int y = 42;
		for (int i = 1; i <= episode_max; i++)
		{
			JE_outTextAdjust(VGAScreen, 20, y, episode_name[i], 15, -4 + (i == episode ? 2 : 0) - (!episodeAvail[i - 1] ? 4 : 0), SMALL_FONT_SHAPES, true);

#ifdef ENABLE_DEVTOOLS
			if (i == episode && mainLevel != FIRST_LEVEL)
			{
				sprintf(tmpBuf.l, "... starting from section ~%hu~", mainLevel);
				JE_textShade(VGAScreen, 20, y + 8, tmpBuf.l, 15, 0, FULL_SHADE);
			}
#endif

			y += (episode_max == 5) ? 18 : 24;
		}

		_regularMenuBehaviors();

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		uint button = 0;

#ifdef ENABLE_DEVTOOLS
		if (inputFuzzing)
			selection_made = true;
		else
#endif
		if (_outOfTime) // Timer expired?
			selection_made = true;
		else while (I_inputForMenu(&button, INPUT_P2_FIRE))
		{
			JE_byte p = (button >= INPUT_P2_UP) ? 1 : 0;
			switch (button++)
			{
#ifdef ENABLE_DEVTOOLS
			case INPUT_P1_RIGHT:
			case INPUT_P2_RIGHT:
				if (in_control != p)
					break;
				mainLevel++;
				JE_playSampleNum(S_CURSOR);
				break;

			case INPUT_P1_LEFT:
			case INPUT_P2_LEFT:
				if (in_control != p)
					break;
				mainLevel--;
				JE_playSampleNum(S_CURSOR);
				break;
#endif

			case INPUT_P1_FIRE:
			case INPUT_P2_FIRE:
				if (player[p].player_status != STATUS_SELECT && ARC_CoinStart(&player[p]))
				{
					JE_playSampleNum(S_SELECT);
					player[p].player_status = STATUS_SELECT;
				}
				else if (in_control == p)
					selection_made = true;
				break;

			case INPUT_P1_UP:
			case INPUT_P2_UP:
				if (in_control != p)
					break;
				do
				{
					if (--episode < 1)
						episode = episode_max;
				} while (!episodeAvail[episode - 1]);
				JE_playSampleNum(S_CURSOR);
#ifdef ENABLE_DEVTOOLS
				mainLevel = FIRST_LEVEL;
#endif
				break;
			case INPUT_P1_DOWN:
			case INPUT_P2_DOWN:
				if (in_control != p)
					break;
				do
				{
					if (++episode > episode_max)
						episode = 1;
				} while (!episodeAvail[episode - 1]);
				JE_playSampleNum(S_CURSOR);
#ifdef ENABLE_DEVTOOLS
				mainLevel = FIRST_LEVEL;
#endif
				break;

			default:
				break;
			}
		}
	}

	if (!episodeAvail[episode - 1])
		episode = 1;
	JE_playSampleNum(S_SELECT);
	fade_black(10);

	Episode_init(episode);
	initial_episode_num = episodeNum;
}

// Shown on episode change
void Menu_newEpisode( void )
{
	int episode_max = EPISODE_AVAILABLE;
	if (!episodeAvail[4]) // If episode 5 isn't available, don't show it.
		--episode_max;

	play_song(SONG_NEXTEPISODE);

	JE_loadPCX(arcdata_dir(), "select.pcx");
	JE_dString(VGAScreen, JE_fontCenter("Next Episode", FONT_SHAPES), 16, "Next Episode", FONT_SHAPES);
	for (int i = 1, y = 42; i <= episode_max; i++)
	{
		JE_outTextAdjust(VGAScreen, 20, y, episode_name[i], 15, -4 + (episodeNum == i ? 2 : -4), SMALL_FONT_SHAPES, true);
		y += (episode_max == 5) ? 18 : 24;
	}

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	mainLevel = FIRST_LEVEL;

	_enterMenuBehaviors(10);
	for (; ; )
	{
		setjasondelay(2);

		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (int p = 0; p < 2; ++p)
		{
			if (player[p].player_status != STATUS_SELECT)
				_drawPlayerStatusText(p);
		}

		_regularMenuBehaviors();

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		if (player[0].player_status == STATUS_NONE && I_inputMade(INPUT_P1_FIRE) && ARC_CoinStart(&player[0]))
		{
			player[0].player_status = STATUS_SELECT;
			break;
		}
		else if (player[1].player_status == STATUS_NONE && I_inputMade(INPUT_P2_FIRE) && ARC_CoinStart(&player[1]))
		{
			player[1].player_status = STATUS_SELECT;
			break;
		}
		else if (_outOfTime 
		 || (player[0].player_status == STATUS_INGAME && I_inputMade(INPUT_P1_FIRE))
		 || (player[1].player_status == STATUS_INGAME && I_inputMade(INPUT_P2_FIRE)))
			break;
	}
	JE_playSampleNum(S_SELECT);

	// If someone joined *now*, open the select ship screen.
	if (player[0].player_status == STATUS_SELECT || player[1].player_status == STATUS_SELECT)
	{
		fade_black(10);
		Menu_selectShip();
	}
	fade_black(15);
}


//
// End episode interlude
//

void Menu_episodeInterlude( bool silent )
{
	Uint64 endTime;

	if (!silent) // ep4 should let the previous sad music continue
		play_song(SONG_EPISODEEND);
	endTime = SDL_GetTicks() + 4300; // Note: Synced with music

	hasRequestedToSkip = false;

	setjasondelay2(6);
	frameCountMax = 2;

	JE_loadPCX(arcdata_dir(), "select.pcx");
	memcpy(colors, palettes[0], sizeof(colors)); // Force normal palette without weird 254th color nonsense
	JE_dString(VGAScreen, JE_fontCenter("Episode Complete", FONT_SHAPES), 16, "Episode Complete", FONT_SHAPES);
	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	for (int p = 0; p < 2; ++p)
	{
		if (player[p].player_status == STATUS_INGAME)
			_drawPlayerStatusText(p);
		else
			_menuMirrorText(VGAScreen, 12, 136, p, "Please Wait");
	}

	fade_palette(colors, 10, 0, 255);

	textGlowFont = SMALL_FONT_SHAPES;
	int num_bonuses = (episodeNum == 5 || !episodeAvail[episodeNum]) ? 2 : 1;
	for (int bonus = 1; bonus <= num_bonuses; ++bonus)
	{
		// This is hackish, but: We only want to undo the area where the player's score is
		memcpy((unsigned char*)VGAScreen->pixels + (VGAScreen->pitch * 130),
		       (unsigned char*)VGAScreen2->pixels + (VGAScreen->pitch * 130),
		       VGAScreen->pitch * 60);

		int y = 10 + (40 * bonus);
		if (bonus == 1)
			JE_saveTextGlow(JE_fontCenter("Episode completion bonus.", SMALL_FONT_SHAPES), y, "Episode completion bonus.");
		else
			JE_saveTextGlow(JE_fontCenter("Bonus for remaining lives.", SMALL_FONT_SHAPES), y, "Bonus for remaining lives.");

		for (int p = 0; p < 2; ++p)
		{
			if (player[p].player_status != STATUS_INGAME)
			{
				_menuMirrorText(VGAScreen, 12, 136, p, "Please Wait");
				continue;
			}

			int bonus_value = (bonus == 1) ? 10000 : 10000 * player[p].lives;
			player[p].cash += bonus_value;

			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%d", bonus_value);
			int x = (p == 0) ? 50 : 270 - JE_textWidth(tmpBuf.s, SMALL_FONT_SHAPES);

			JE_saveTextGlow(x, y + 15, tmpBuf.s);
			_drawPlayerStatusText(p);
		}
		JE_drawTextGlow(VGAScreenSeg);
	}

	do
	{
		setjasondelay(2);
		JE_showVGA();
		wait_delay();

		if (I_checkSkipScene())
		{
			JE_playSampleNum(S_SELECT);
			break;
		}
	} while (!(SDL_GetTicks() > endTime));

	// Second section -- hint for ship twiddles, or generic episode hint
	hasRequestedToSkip = false;
	memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

	for (int p = 0; p < 2; ++p)
	{
		if (player[p].player_status == STATUS_INGAME)
			_drawPlayerStatusText(p);
		else
			_menuMirrorText(VGAScreen, 12, 136, p, "Please Wait");
	}

	// Choose a hint; go through all hints, find ones applicable to us, and keep tally.
	// Then randomly choose a number between 0 and the number of good hints, and go through again.
	int goodHints = 0;
	int hintsToSkip = 0;
	int chosenHint = 0;
	for (size_t i = 0; i < num_hints; ++i)
	{
		if (hints[i].reference == 0xF0 + episodeNum)
			++goodHints;
		else if ((player[0].player_status == STATUS_INGAME && hints[i].reference == player[0].items.ship)
		      || (player[1].player_status == STATUS_INGAME && hints[i].reference == player[1].items.ship))
		{
			// Ship hints are more likely.
			goodHints += 4;
		}
	}
	if (goodHints == 0) // If NOTHING fits, pick anything at random.
		chosenHint = mt_rand() % num_hints;
	else
	{
		hintsToSkip = mt_rand() % goodHints;
		for (size_t i = 0; i < num_hints; ++i)
		{
			if (hints[i].reference == 0xF0 + episodeNum)
			{
				if (--hintsToSkip < 0)
				{
					chosenHint = i;
					break;
				}
			}
			else if ((player[0].player_status == STATUS_INGAME && hints[i].reference == player[0].items.ship)
			      || (player[1].player_status == STATUS_INGAME && hints[i].reference == player[1].items.ship))
			{
				// Ship hints are more likely.
				if ((hintsToSkip -= 4) < 0)
				{
					chosenHint = i;
					break;
				}
			}
		}
	}

	// Now display the hint!
	if (hints[chosenHint].reference >= 0xF0)
	{
		JE_saveTextGlow(JE_fontCenter(episode_name[hints[chosenHint].reference - 0xF0], SMALL_FONT_SHAPES),
			38, episode_name[hints[chosenHint].reference - 0xF0]);
	}
	else
	{
		strcpy(tmpBuf.l, JE_trim(ships[hints[chosenHint].reference].name));
		JE_saveTextGlow(JE_fontCenter(tmpBuf.l, SMALL_FONT_SHAPES), 38, tmpBuf.l);
	}
	JE_saveTextGlow(JE_fontCenter("Secret Hint", SMALL_FONT_SHAPES), 50, "Secret Hint");
	JE_drawTextGlow(VGAScreenSeg);
	useLastBank = true;
	warningRed = false;

	tempY = 57;
	for (int i = 0; i < 7; i++)
		JE_outCharGlow(10, (tempY += 9), hints[chosenHint].text[i]);

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);
	_enterMenuBehaviors(10);
	_fadeIn = false;	
	do
	{
		setjasondelay(2);
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		_regularMenuBehaviors();
		JE_showVGA();
		wait_delay();
	} while (!I_checkSkipScene() && !_outOfTime);
	JE_playSampleNum(S_SELECT);
	fade_black(10);
}

//
// Title screen
//

bool Menu_titleScreen( void )
{
	uint button;
	int tyrY = 62, t2kY = 41;
	bool fadeIn = true;

	JE_word waitForDemo = 1; // will get overridden immediately
	JE_word oldCoins = -1;
	JE_word curCoins;

	skip_header_draw = skip_header_undraw = false;
	play_demo = false;
	gameLoaded = false;

	// ARCADE TITLE SCREEN STARTUP
	set_volume(tyrMusicVolume, fxVolume);
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

	if (true) // Always move the logo up
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

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);
	do
	{
		setjasondelay(2);
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen2->pitch * VGAScreen2->h);

		int x = VGAScreen->w / 2, y = 110;
		if (tyrian2000detected)
			y += 32;

		curCoins = ARC_GetCoins();

		strcpy(tmpBuf.s, (curCoins >= DIP.coinsToStart) ? "Press Fire" : "Insert Coin");
		draw_font_hv(VGAScreen, x - 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x - 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x,     y,     tmpBuf.s, normal_font, centered, 15, -3);
		y += 16;

		if (DIP.coinsToStart == 0)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Free Play");
		else if (DIP.coinsToStart == 1)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu", curCoins);
		else
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu/%hu", curCoins, DIP.coinsToStart);
		draw_font_hv(VGAScreen, x - 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x + 1, y - 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x - 1, y + 1, tmpBuf.s, normal_font, centered, 15, -10);
		draw_font_hv(VGAScreen, x,     y,     tmpBuf.s, normal_font, centered, 15, -3);

		if (DIP.coinsToStart != DIP.coinsToContinue)
		{
			snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%hu Credit%s To Start", DIP.coinsToStart, DIP.coinsToStart == 1 ? "" : "s");
			draw_font_hv_full_shadow(VGAScreen, x, 186, tmpBuf.l, small_font, centered, 15, 2, true, 1);
			snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%hu Credit%s To Continue", DIP.coinsToContinue, DIP.coinsToContinue == 1 ? "" : "s");
			draw_font_hv_full_shadow(VGAScreen, x, 193, tmpBuf.l, small_font, centered, 15, 2, true, 1);
		}

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
				if (ARC_CoinStart(&player[1]))
					player[1].player_status = STATUS_SELECT;
				break;
			case INPUT_P1_FIRE:
				if (ARC_CoinStart(&player[0]))
					player[0].player_status = STATUS_SELECT;
				break;
			}
		}

		if (player[0].player_status == STATUS_SELECT || player[1].player_status == STATUS_SELECT)
		{
			JE_playSampleNum(S_SELECT);
			fade_black(10);

			// Don't draw the header for menu stuff anymore.
			// All the relevant information is shown already.
			skip_header_draw = true;
			Menu_selectEpisode();
			Menu_selectShip();

			// Start special mode!
			currentRank = 0;
			initialDifficulty = difficultyLevel = DIP.startingDifficulty;
			gameLoaded = true;
		}

		if (oldCoins != curCoins)
		{
			oldCoins = curCoins;
			waitForDemo = (oldCoins >= DIP.coinsToStart) ? 35*10 : 35*5;
		}
		else
			--waitForDemo;
	}
	while (!gameLoaded && waitForDemo);

	fade_black(15);
	return gameLoaded;
}

void Menu_hotDebugScreen( void )
{
	const char *menu_strings[] = {
		"Screenshot Mode",
		"Music Volume",
		"Sound Volume",
		"Detail Level",
		"Game Speed",

		"P1 Weapon",
		"P1 Power",
		"P2 Weapon",
		"P2 Power",

		"Debug Displays",
		"Invulnerability",
		"Skip Level",
		"Skip Level (Fail)",
		"Return to Title",
	};
	SDL_Surface *temp_surface = VGAScreen;
	VGAScreen = VGAScreenSeg; /* side-effect of game_screen */

	JE_byte sel = 1, plnum;
	JE_boolean screenshot_pause = false;

	// Stops input fuzzing
	inServiceMenu = true;

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	while (true)
	{
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		if (screenshot_pause)
		{
			// No-op
		}
		else if (DIP.enableFullDebugMenus)
		{
			JE_barShade(VGAScreen, 31, 53, 217+36, 182); /*Main Box*/
			JE_barShade(VGAScreen, 33, 55, 215+36, 180);

			for (x = 0; x < 5; x++)
				JE_outTextAdjust(VGAScreen, 38, 60 + (x*8), menu_strings[x], 15, ((sel == x+1) << 1) + 2, TINY_FONT, true);
			for (x = 5; x < 9; x++)
				JE_outTextAdjust(VGAScreen, 38, 62 + (x*8), menu_strings[x], 15, ((sel == x+1) << 1) + 2, TINY_FONT, true);
			for (x = 9; x < 14; x++)
				JE_outTextAdjust(VGAScreen, 38, 64 + (x*8), menu_strings[x], 15, ((sel == x+1) << 1) + 2, TINY_FONT, true);

			JE_barDrawShadow(VGAScreen, 140, 60 + (1*8), 1, music_disabled ? 12 : 16, tyrMusicVolume / 12, 3, 6);
			JE_barDrawShadow(VGAScreen, 140, 60 + (2*8), 1, samples_disabled ? 12 : 16, fxVolume / 12, 3, 6);
			JE_outTextAdjust(VGAScreen, 140, 60 + (3*8), detailLevel[processorType-1], 15, ((sel == 4) << 1) + 2, TINY_FONT, true);
			JE_outTextAdjust(VGAScreen, 140, 60 + (4*8), gameSpeedText[gameSpeed-1],   15, ((sel == 5) << 1) + 2, TINY_FONT, true);

			JE_outTextAdjust(VGAScreen, 140, 62 + (5*8), weaponPort[player[0].cur_item.weapon].name, 15, ((sel == 6) << 1) + 2, TINY_FONT, true);
			JE_barDrawShadow(VGAScreen, 140, 62 + (6*8), 1, player[0].player_status != STATUS_INGAME ? 12 : 16, player[0].items.power_level, 3, 6);
			JE_outTextAdjust(VGAScreen, 140, 62 + (7*8), weaponPort[player[1].cur_item.weapon].name, 15, ((sel == 8) << 1) + 2, TINY_FONT, true);
			JE_barDrawShadow(VGAScreen, 140, 62 + (8*8), 1, player[1].player_status != STATUS_INGAME ? 12 : 16, player[1].items.power_level, 3, 6);
		}
		else
		{
			JE_barShade(VGAScreen, 31, 131, 217+36, 182); /*Main Box*/
			JE_barShade(VGAScreen, 33, 133, 215+36, 180);
			JE_outTextAdjust(VGAScreen, 38, 138 + (0*8), menu_strings[0], 15,  ((sel == 1)  << 1) + 2, TINY_FONT, true);
			JE_outTextAdjust(VGAScreen, 38, 138 + (1*8), menu_strings[1], 15,  ((sel == 2)  << 1) + 2, TINY_FONT, true);
			JE_outTextAdjust(VGAScreen, 38, 138 + (2*8), menu_strings[2], 15,  ((sel == 3)  << 1) + 2, TINY_FONT, true);
			JE_outTextAdjust(VGAScreen, 38, 138 + (3*8), menu_strings[3], 15,  ((sel == 4)  << 1) + 2, TINY_FONT, true);
			JE_outTextAdjust(VGAScreen, 38, 138 + (4*8), menu_strings[13], 15, ((sel == 14) << 1) + 2, TINY_FONT, true);

			JE_barDrawShadow(VGAScreen, 140, 138 + (1*8), 1, music_disabled ? 12 : 16, tyrMusicVolume / 12, 3, 6);
			JE_barDrawShadow(VGAScreen, 140, 138 + (2*8), 1, samples_disabled ? 12 : 16, fxVolume / 12, 3, 6);
			JE_outTextAdjust(VGAScreen, 140, 138 + (3*8), detailLevel[processorType-1], 15, ((sel == 4) << 1) + 2, TINY_FONT, true);
		}

		JE_showVGA();

		tempW = 0;
		uint button = INPUT_P1_UP;
		I_waitOnInputForMenu(button, INPUT_SERVICE_ENTER, 0);
		while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			screenshot_pause = false;
			plnum = 0;

			switch (button++)
			{
			case INPUT_P1_FIRE:
				JE_playSampleNum(S_SELECT);
				switch (sel)
				{
					case 1: screenshot_pause = true; break;
					case 2: music_disabled = !music_disabled; break;
					case 3: samples_disabled = !samples_disabled; break;
					case 11: youAreCheating = !youAreCheating; break;

					case 10: 
						debug = !debug;
						debugHist = 1;
						debugHistCount = 1;

						/* YKS: clock ticks since midnight replaced by SDL_GetTicks */
						lastDebugTime = SDL_GetTicks();
						break;

					case 13:
						levelTimer = true; // incites fail
						// fall through
					case 12:
						levelTimerCountdown = 0;
						endLevel = true;
						levelEnd = 40;
						goto leave_ingame_menu;
					case 14:
						reallyEndLevel = true;
						playerEndLevel = true;
						goto leave_ingame_menu;
				}
				break;
			case INPUT_P1_UP:
				if (--sel < 1) sel = 14;
				else if (!DIP.enableFullDebugMenus && sel >= 5 && sel <= 13) sel = 4;
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_DOWN:
				if (++sel > 14) sel = 1;
				else if (!DIP.enableFullDebugMenus && sel >= 5 && sel <= 13) sel = 14;
				JE_playSampleNum(S_CURSOR);
				break;
			case INPUT_P1_LEFT:
				switch (sel)
				{
					case 2:
						JE_changeVolume(&tyrMusicVolume, -12, &fxVolume, 0);
						if (music_disabled)
						{
							music_disabled = false;
							restart_song();
						}
						break;
					case 3:
						JE_changeVolume(&tyrMusicVolume, 0, &fxVolume, -12);
						samples_disabled = false;
						break;
					case 4:
						if (--processorType < 1)
							processorType = 1;
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
					case 5:
						if (--gameSpeed < 1)
							gameSpeed = 1;
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;

					case 8:
						plnum = 1;
						// fall through
					case 6:
						if (--player[plnum].port_mode == 255)
							player[plnum].port_mode = 4;
						PL_SwitchWeapon(&player[plnum], player[plnum].port_mode, false);
						break;

					case 9:
						plnum = 1;
						// fall through
					case 7:
						if (player[plnum].player_status == STATUS_INGAME && --player[plnum].items.power_level == 0)
							player[plnum].items.power_level = 1;
						break;
				}
				if (sel > 1 && sel < 10)
				{
					JE_playSampleNum(S_CURSOR);
				}
				break;
			case INPUT_P1_RIGHT:
				switch (sel)
				{
					case 2:
						JE_changeVolume(&tyrMusicVolume, 12, &fxVolume, 0);
						if (music_disabled)
						{
							music_disabled = false;
							restart_song();
						}
						break;
					case 3:
						JE_changeVolume(&tyrMusicVolume, 0, &fxVolume, 12);
						samples_disabled = false;
						break;
					case 4:
						if (++processorType > 6)
							processorType = 6;
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;
					case 5:
						if (++gameSpeed > 6)
							gameSpeed = 6;
						JE_initProcessorType();
						JE_setNewGameSpeed();
						break;

					case 8:
						plnum = 1;
						// fall through
					case 6:
						if (++player[plnum].port_mode > 4)
							player[plnum].port_mode = 0;
						PL_SwitchWeapon(&player[plnum], player[plnum].port_mode, false);
						break;

					case 9:
						plnum = 1;
						// fall through
					case 7:
						if (player[plnum].player_status == STATUS_INGAME && ++player[plnum].items.power_level > 11)
							player[plnum].items.power_level = 11;
						break;
				}
				if (sel > 1 && sel < 10)
				{
					JE_playSampleNum(S_CURSOR);
				}
				break;
			case INPUT_SERVICE_HOTDEBUG:
				goto leave_ingame_menu;
			case INPUT_SERVICE_ENTER:
				inServiceMenu = false;
				ARC_EnterService(); // DOES NOT RETURN
			}
		}
	}

leave_ingame_menu:
	VGAScreen = temp_surface; /* side-effect of game_screen */
	inServiceMenu = false;
}
