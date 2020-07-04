/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcade.c
/// \brief Arcade specific functions and data, DIP switches, etc

#include "../arcade.h"
#include "../config.h"
#include "../fonthand.h"
#include "../input.h"
#include "../mainint.h"
#include "../nortsong.h"
#include "../opentyr.h"
#include "../palette.h"
#include "../picload.h"
#include "../player.h"
#include "../sndmast.h"
#include "../tyrian2.h"
#include "../video.h"

#include <ctype.h>
#include <string.h>
#include <setjmp.h>

//
// DIP Switches and phyiscal hardware
//

const DipSwitches DIP_Default = {
	3, 2, 6, 0, 
	2, 3, 3, 3, 3, 
	2
};
DipSwitches DIP;

static JE_word coins = 0;

JE_boolean skipIdentify = false;
static bool inIdentify = false;
static int identifyStrY = 31;

bool attractAudioAllowed = true;

void ARC_IdentifyPrint( const char *s )
{
	if (!inIdentify)
		return;

	JE_textShade(VGAScreen, 4, identifyStrY, s, 8, 2, FULL_SHADE);
	identifyStrY += 9;
	JE_showVGA();
}

void ARC_IdentifyWarn( const char *s )
{
	if (!inIdentify)
		return;

	JE_textShade(VGAScreen, 4, identifyStrY, s, 14, 6, FULL_SHADE);
	identifyStrY += 9;
	JE_showVGA();
}

void ARC_Identify( void )
{
	if (skipIdentify)
		return;

	inIdentify = true;
	skip_header_draw = true;

	JE_loadPic(VGAScreen, 2, true);

	JE_textShade(VGAScreen, 4, 4, "This is...", 8, 2, FULL_SHADE);
	JE_textShade(VGAScreen, JE_fontCenter("-- ArcTyr --", TINY_FONT), 4, "~-- ArcTyr --~", 8, 2, FULL_SHADE);
	snprintf(tmpBuf.l, sizeof(tmpBuf.l), "Based on OpenTyrian, rev. %s", opentyrian_version);
	JE_textShade(VGAScreen, 4, 13, tmpBuf.l, 8, 2, FULL_SHADE);
	snprintf(tmpBuf.l, sizeof(tmpBuf.l), "Compiled: %s %s", opentyrian_date, opentyrian_time);
	JE_textShade(VGAScreen, 4, 22, tmpBuf.l, 8, 2, FULL_SHADE);

	JE_showVGA();

	JE_playSampleNumOnChannel((tyrian2000detected) ? V_DANGER : V_GOOD_LUCK, SFXPRIORITY+2);
}

void ARC_IdentifyEnd( void )
{
	while (SDL_GetTicks() < 3000)
	{
		I_checkButtons();
		SDL_Delay(1);
	}
	skip_header_draw = false;
	inIdentify = false;
	fade_black(1);
}

void ARC_InsertCoin( void )
{
	if (++coins > 10000)
		coins = 10000;
	JE_playSampleNumOnChannel(S_ITEM, SFXPRIORITY+6);
	if (DIP.coinsPerGame && !(coins % DIP.coinsPerGame))
	{
		if (play_demo)
			reallyEndLevel = true;
		JE_playSampleNumOnChannel(V_DATA_CUBE, SFXPRIORITY+2);
	}

	arcTextTimer = 0;

	for (int p = 0; p < 2; ++p)
	{
		// If continuing when a coin is inserted,
		// reset the timer back to 10 to be nice
		if (player[p].player_status == STATUS_CONTINUE)
		{
			player[p].arc.timerFrac = 0;
			player[p].arc.timer = 10;
		}
	}
}

JE_boolean ARC_CoinStart( void )
{
	if (isInGame && (normalBonusLevelCurrent || bonusLevelCurrent))
		return false; // Can't join in the middle of a bonus level!
	if (coins < DIP.coinsPerGame)
		return false;
	coins -= DIP.coinsPerGame;
	return true;
}

void ARC_GetCredits( JE_word *cFull, JE_word *cPartial, JE_word *cpg )
{
	if (DIP.coinsPerGame == 0)
		return;
	if (cFull)
		*cFull = coins / DIP.coinsPerGame;
	if (cPartial)
		*cPartial = coins % DIP.coinsPerGame;
	if (cpg)
		*cpg = (JE_word)DIP.coinsPerGame;
}

JE_word ARC_GetCoins( void )
{
	return coins;
}



//
// High Scores
//
HighScoreEntry highScores[20] = {
	{"Trent    ", 100000, false},
	{"Dougan   ",  90000, false},
	{"Transon  ",  80000, false},
	{"Vykromod ",  70000, false},
	{"Beppo    ",  60000, false},
	{"Steffan  ",  50000, false},
	{"Lori     ",  45000, false},
	{"Angel    ",  40000, false},
	{"Jazz     ",  35000, false},
	{"Crystal  ",  30000, false},
	{"Javi     ",  25000, false},
	{"Cossette ",  20000, false},
	{"Jill     ",  15000, false},
	{"Raven    ",  10000, false},
	{"Spaz     ",   7000, false},
	{"849      ",   5000, false},
	{"Darcy    ",   4000, false},
	{"Jean-Paul",   3000, false},
	{"Jake     ",   2000, false},
	{"Devan    ",   1000, false},
};

JE_boolean ARC_HS_IsLeading( uint cash )
{
	return (cash >= highScores[0].cash);
}

JE_boolean ARC_HS_InsertName( Player *pl )
{
	for (int i = 0; i < 20; ++i)
	{
		if (pl->cash >= highScores[i].cash)
		{
			// Shift down high scores if necessary
			if (i < 19)
				memmove(&highScores[i+1], &highScores[i], sizeof(HighScoreEntry) * (19 - i));
			snprintf(highScores[i].name, sizeof(highScores[i].name), "Unknown");
			highScores[i].cash = pl->cash;
			highScores[i].new = true;

			pl->arc.hsPos = i;
			{
				Player *otherPl = PL_OtherPlayer(pl);
				if (otherPl->arc.hsPos >= i)
					++otherPl->arc.hsPos;
			}
			return true;
		}
	}
	return false;
}

//
// Arcade text displays
//

JE_boolean playingCredits = false;
JE_byte arcTextTimer = 0;

#define HSALPHABET_MIDPOINT 43
#define HSALPHABET_END      86

static const char *arcHighScoreAlphabet =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?!0123456789\x1E\x1F "
	"abcdefghijklmnopqrstuvwxyz.,?!0123456789\x1E\x1F ";

void ARC_DISP_NoPlayerInSlot( uint pNum )
{
	int x;
	JE_byte color = 15;
	JE_byte brightness;

	if (isInGame && (normalBonusLevelCurrent || bonusLevelCurrent))
	{
		strcpy(tmpBuf.s, "Please Wait");
	}
	else if (arcTextTimer >= 100)
	{
		if (coins < DIP.coinsPerGame)
			strcpy(tmpBuf.s, "Insert Coin");
		else
			strcpy(tmpBuf.s, "Press Fire");
	}
	else
	{
		if (DIP.coinsPerGame == 0)
			strcpy(tmpBuf.s, "Free Play");
		else 
		{
			JE_word cFull = 1, cPartial = 0;
			ARC_GetCredits(&cFull, &cPartial, NULL);
			if (!cPartial)
				snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu", cFull);
			else
				snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Credits %hu %hu/%hu", cFull, (JE_word)cPartial, (JE_word)DIP.coinsPerGame);
		}
	}

	switch (arcTextTimer % 100)
	{
		case 21:
		case 22:
		case 3:
		case 4: brightness = 2; break;
		case 19:
		case 20:
		case 5:
		case 6: brightness = 3; break;
		case 17:
		case 18:
		case 7:
		case 8: brightness = 4; break;
		case 15:
		case 16:
		case 9:
		case 10: brightness = 5; break;
		case 11:
		case 12:
		case 13:
		case 14: brightness = 6; break;
		default: brightness = 1; break;
	}

	x = (pNum == 2) ? 236 - JE_textWidth(tmpBuf.s, TINY_FONT) : 30;
	JE_textShade(VGAScreen, 28+x, 4, tmpBuf.s, color, brightness, FULL_SHADE);
}

void ARC_DISP_InGameDisplay( uint pNum )
{
	int x;

	strcpy(tmpBuf.l, JE_trim(ships[player[pNum - 1].items.ship].name));
	x = (pNum == 2) ? 264 - JE_textWidth(tmpBuf.l, TINY_FONT) : 58;
	JE_textShade(VGAScreen, x, 4, tmpBuf.l, 2, 6, FULL_SHADE);

	sprintf(tmpBuf.s, "%lu", player[pNum - 1].cash);
	x = (pNum == 2) ? 264 - JE_textWidth(tmpBuf.s, TINY_FONT) : 58;
	JE_textShade(VGAScreen, x, 13, tmpBuf.s, ARC_HS_IsLeading(player[pNum -1].cash) ? 15 : 2, 4, FULL_SHADE);
}

void ARC_DISP_HighScoreEntry( uint pNum )
{
	Sint8 vBright = -4;
	JE_byte position = player[pNum - 1].arc.hsPos + 1;

	x = 58;
	if (position == 1)
		strcpy(tmpBuf.s, "1st");
	else if (position == 2)
		strcpy(tmpBuf.s, "2nd");
	else if (position == 3)
		strcpy(tmpBuf.s, "3rd");
	else if (position == 0 || position > 20)
		strcpy(tmpBuf.s, "21st");
	else
		snprintf(tmpBuf.s, 5, "%dth", position);
	JE_textShade(VGAScreen, x, 4, tmpBuf.s, 15, 4, FULL_SHADE);

	x = 80;

	if (player[pNum - 1].arc.timer >= 65465)
	{
		vBright += (player[pNum - 1].arc.timer & 6) >> 1;
		if (vBright == -1)
			vBright = -3;
	}

	JE_outTextAdjust(VGAScreen, x, 4, player[pNum - 1].arc.hsName, 15, vBright, SMALL_FONT_SHAPES, true);

	if (player[pNum - 1].arc.timer >= 65465)
		return;

	x += JE_textWidth(player[pNum - 1].arc.hsName, SMALL_FONT_SHAPES);
	vBright = (player[pNum - 1].arc.timer % 35 > 17) ? -2 : -3;
	switch (arcHighScoreAlphabet[player[pNum - 1].arc.cursor])
	{
		case 0x1E:
			JE_textShade(VGAScreen, x,   4, "B", 15, 6 + vBright, FULL_SHADE);
			JE_textShade(VGAScreen, x+5, 8, "S", 15, 6 + vBright, FULL_SHADE);
			break;
		case 0x1F:
			JE_textShade(VGAScreen, x,   4, "E", 15, 6 + vBright, FULL_SHADE);
			JE_textShade(VGAScreen, x+5, 8, "D", 15, 6 + vBright, FULL_SHADE);
			break;
		case 0x20:
			JE_textShade(VGAScreen, x,   4, "S", 15, 6 + vBright, FULL_SHADE);
			JE_textShade(VGAScreen, x+5, 8, "P", 15, 6 + vBright, FULL_SHADE);
			break;
		default:
			sprintf(tmpBuf.s, "%c", arcHighScoreAlphabet[player[pNum - 1].arc.cursor]);
			JE_outTextAdjust(VGAScreen, x, 4, tmpBuf.s, 15, vBright, SMALL_FONT_SHAPES, true);
			break;
	}
}

void ARC_DISP_ContinueEntry( uint pNum )
{
	x = (pNum == 2) ? (264 - JE_textWidth("Continue?", TINY_FONT)) : 58;
	JE_textShade(VGAScreen, x, 4, "Continue?", 15, 3, FULL_SHADE);

	sprintf(tmpBuf.s, "%hu", player[pNum - 1].arc.timer);
	x = (pNum == 2) ? (210 - JE_textWidth(tmpBuf.s, SMALL_FONT_SHAPES)) : 112;
	JE_dString (VGAScreen, x, 4, tmpBuf.s, SMALL_FONT_SHAPES);

}

void ARC_DISP_MidGameSelect( uint pNum )
{
	uint sGr = ships[player[pNum - 1].items.ship].shipgraphic;
	if (sGr == 0) // Dragonwing
	{
		blit_sprite2x2(VGAScreen, (pNum == 2) ? 241 : 29, 1, shapes9, 13);
		blit_sprite2x2(VGAScreen, (pNum == 2) ? 265 : 53, 1, shapes9, 51);
	}
	else if (sGr == 1) // Nortship
	{
		blit_sprite2x2(VGAScreen, (pNum == 2) ? 241 : 29, 1, shapes9, 318);
		blit_sprite2x2(VGAScreen, (pNum == 2) ? 265 : 53, 1, shapes9, 320);
	}
	else
		blit_sprite2x2(VGAScreen, (pNum == 2) ? 265 : 29, 1, shapes9, sGr);

	strcpy(tmpBuf.l, JE_trim(ships[player[pNum - 1].items.ship].name));
	x = (pNum == 2) ? (264 - JE_textWidth(tmpBuf.l, TINY_FONT)) : 58;
	JE_textShade(VGAScreen, x, 20, tmpBuf.l, 2, 6, FULL_SHADE);

	x = (pNum == 2) ? (264 - JE_textWidth("Select Ship", TINY_FONT)) : 58;
	JE_textShade(VGAScreen, x, 4, "Select Ship", 15, 3, FULL_SHADE);

	sprintf(tmpBuf.s, "%hu", player[pNum - 1].arc.timer);
	x = (pNum == 2) ? (210 - JE_textWidth(tmpBuf.s, SMALL_FONT_SHAPES)) : 112;
	JE_dString (VGAScreen, x, 4, tmpBuf.s, SMALL_FONT_SHAPES);
}

void ARC_DISP_GameOverHold( uint pNum )
{
	if (playingCredits)
		return;

	x = (pNum == 2) ? 236 - JE_textWidth("Game Over", TINY_FONT) : 30;
	JE_textShade(VGAScreen, 28+x, 4, "Game Over", 15, 3, FULL_SHADE);
}

void ARC_SetPlayerStatus( Player *pl, int status )
{
	pl->arc.timer = 0;
	pl->arc.timerFrac = 0;
	pl->arc.hsPos = -1;

	// Special handling needed?
	switch (status)
	{
	//
	// GAME OVER
	//
	case STATUS_GAMEOVER:
		// Erase stored name
		pl->arc.hsName[0] = '\0';
		break;

	//
	// SHIP SELECTION
	//
	case STATUS_SELECT:
		pl->arc.timer = 20;

		// if continuing: quickly find what our current ship is and preselect it
		if (pl->player_status == STATUS_CONTINUE)
		{
			for (uint i = 0; i < COUNTOF(shiporder); ++i)
			{
				if (shiporder[i] == pl->items.ship)
				{
					pl->arc.cursor = i;
					break;
				}
			}
			// ERROR?
		}
		else // if not: select the first ship (or the second, if the first is in use)
		{
			Player *otherPl = PL_OtherPlayer(pl);
			pl->items.ship = shiporder[(pl->arc.cursor = 0)];
			if (otherPl->player_status > STATUS_NONE && otherPl->items.ship == shiporder[0])
				pl->items.ship = shiporder[(pl->arc.cursor = 1)];	
		}
		break;

	//
	// HIGH SCORE NAME ENTRY
	//
	case STATUS_NAMEENTRY:
		// Demo: skip directly to GAMEOVER
		if (play_demo || record_demo)
		{
			pl->player_status = STATUS_GAMEOVER;
			return;
		}
		// Failed to enter high scores -- skip name entry?
		if (!ARC_HS_InsertName(pl))
		{
			pl->arc.timer = 10;
			pl->player_status = (playingCredits) ? STATUS_GAMEOVER : STATUS_CONTINUE;
			return;
		}
		pl->arc.timer = 35 * 60;
		pl->arc.timerFrac = 35 * 15;

		// If the player has a name already, default to END, otherwise A
		pl->arc.cursor = (pl->arc.hsName[0]) ? 41 : 0;
		break;

	//
	// CONTINUE COUNTDOWN
	//
	case STATUS_CONTINUE:
		// Not if the game's already over you aren't!
		if (playingCredits)
		{
			pl->player_status = STATUS_GAMEOVER;
			return;
		}

		pl->arc.timer = 10;
		break;

	default:
		break;
	}

	pl->player_status = status;
}

// Handles any statuses that aren't INGAME
void ARC_HandlePlayerStatus( Player *pl, uint pNum )
{
	Player *otherPl = PL_OtherPlayer(pl);

	// due to using menu input, this only tracks
	// buttons PRESSED or REPEATED
	bool buttons[NUM_BUTTONS] = {false};
	uint start = (pNum == 1) ? INPUT_P1_UP : INPUT_P2_UP;
	uint b = start;
	uint end = b + NUM_BUTTONS - 1;
	while (I_inputForMenu(&b, end))
		buttons[(b++) - start] = true;

	switch(pl->player_status)
	{
		//
		// HIGH SCORE NAME ENTRY
		//
		case STATUS_NAMEENTRY:
			--pl->arc.timer;
			--pl->arc.timerFrac;
			if (pl->arc.timer >= 65465)
				break;
			else if (pl->arc.timer > 60000)
			{
				// Assign name to score
				if (pl->arc.hsPos >= 0 && pl->arc.hsPos < 20)
					memcpy(highScores[pl->arc.hsPos].name, pl->arc.hsName, sizeof(pl->arc.hsName));
				ARC_SetPlayerStatus(pl, STATUS_CONTINUE);
				break;
			}

			if (buttons[BUTTON_LEFT])
			{
				if (--pl->arc.cursor == 255)
					pl->arc.cursor = HSALPHABET_END - 1;
				JE_playSampleNumOnChannel(S_CURSOR, SFXPRIORITY+7);

				// Only allow RUB/END if string full
				if (pl->arc.hsName[8] && arcHighScoreAlphabet[pl->arc.cursor] > 0x1F)
					pl->arc.cursor += 2;

				// Skip RUB if string empty
				else if (!pl->arc.hsName[0] && arcHighScoreAlphabet[pl->arc.cursor] == 0x1E)
					--pl->arc.cursor;
			}
			else if (buttons[BUTTON_RIGHT])
			{
				if (++pl->arc.cursor >= HSALPHABET_END)
					pl->arc.cursor = 0;
				JE_playSampleNumOnChannel(S_CURSOR, SFXPRIORITY+7);

				// Only allow RUB/END if string full
				if (pl->arc.hsName[8] && arcHighScoreAlphabet[pl->arc.cursor] > 0x1F)
					pl->arc.cursor -= 2;

				// Skip RUB if string empty
				else if (!pl->arc.hsName[0] && arcHighScoreAlphabet[pl->arc.cursor] == 0x1E)
					++pl->arc.cursor;
			}
			else if (buttons[BUTTON_UP] || buttons[BUTTON_DOWN])
			{
				if (pl->arc.cursor >= HSALPHABET_MIDPOINT)
					pl->arc.cursor -= HSALPHABET_MIDPOINT;
				else
					pl->arc.cursor += HSALPHABET_MIDPOINT;
				JE_playSampleNumOnChannel(S_CURSOR, SFXPRIORITY+7);
			}

			if (buttons[BUTTON_FIRE])
			{
				if (arcHighScoreAlphabet[pl->arc.cursor] == 0x1E) // RUB
				{
					JE_playSampleNumOnChannel(S_SPRING, SFXPRIORITY+7);
					if (pl->arc.hsName[0]) // Don't RUB nothing
					{
						pl->arc.hsName[strlen(pl->arc.hsName) - 1] = 0;
						if (!pl->arc.hsName[0]) // If the string's empty now
							++pl->arc.cursor; // Move off RUB
					}
					break;
				}

				JE_playSampleNumOnChannel(S_SELECT, SFXPRIORITY+7);
				if (arcHighScoreAlphabet[pl->arc.cursor] == 0x1F) // END
				{
					pl->arc.timer = 65535;
					break;
				}

				if (!pl->arc.hsName[8]) // Don't overflow the string (this should never happen but sanity checking)
					pl->arc.hsName[strlen(pl->arc.hsName)] = arcHighScoreAlphabet[pl->arc.cursor];

				if (pl->arc.hsName[8]) // If the string is now full
					pl->arc.cursor = 41; // Move the cursor to END
			}
			else if (buttons[BUTTON_MODE] || buttons[BUTTON_SKICK])
			{
				JE_playSampleNumOnChannel(S_SPRING, SFXPRIORITY+7);
				if (pl->arc.hsName[0]) // Don't RUB nothing
				{
					pl->arc.hsName[strlen(pl->arc.hsName) - 1] = 0;
					if (!pl->arc.hsName[0]) // If the string's empty now
						++pl->arc.cursor; // Move off RUB
				}
				break;
			}
			break;

		//
		// CONTINUE COUNTDOWN
		//
		case STATUS_CONTINUE:
			if (++pl->arc.timerFrac >= 50
				|| (pl->arc.timer < 9 && (buttons[BUTTON_MODE] || buttons[BUTTON_SKICK])))
			{
				if (--pl->arc.timer == 65535)
					ARC_SetPlayerStatus(pl, STATUS_GAMEOVER);
				else
					JE_playSampleNumOnChannel(S_CLICK, SFXPRIORITY+7);
				pl->arc.timerFrac = 0;
				break;
			}
			// fall through

		//
		// EMPTY PLAYER SLOT
		//
		case STATUS_NONE:
			if (gameNotOverYet && buttons[BUTTON_FIRE] && ARC_CoinStart())
			{
				JE_playSampleNumOnChannel(S_SELECT, SFXPRIORITY+7);
				ARC_SetPlayerStatus(pl, STATUS_SELECT);
			}
			break;

		//
		// GAME OVER
		//
		case STATUS_GAMEOVER:
			if (playingCredits)
				break;

			if (++pl->arc.timer >= 140)
				ARC_SetPlayerStatus(pl, STATUS_NONE);
			break;

		//
		// SHIP SELECTION
		//
		case STATUS_SELECT:
			if (++pl->arc.timerFrac >= 35)
			{
				--pl->arc.timer;
				pl->arc.timerFrac = 0;
			}
			if (pl->arc.timer == 65535 || buttons[BUTTON_FIRE])
			{
				JE_playSampleNumOnChannel(S_SELECT, SFXPRIORITY+7);

				// sets STATUS_INGAME
				PL_Init(pl, pl->items.ship, true);

				pl->armor = pl->initial_armor = ships[pl->items.ship].dmg;
				pl->shield_max = shields[pl->items.shield].mpwr * 2;
				pl->shield = pl->shield_max / 2;

				pl->y = 160;
				pl->invulnerable_ticks = 100;
				pl->is_alive = true;

				JE_updateOption(pl, 0);
				JE_updateOption(pl, 1);

				pl->shot_multi_pos[SHOT_NORMAL] = 0;
				pl->shot_repeat[SHOT_NORMAL] = 10;
				if (pl->shot_repeat[SHOT_SPECIAL] == 0)
				{
					pl->shot_repeat[SHOT_SPECIAL] = 2;
					pl->hud_ready_timer = 0;
					pl->hud_repeat_start = 1;
				}

				JE_drawShield();
				JE_drawArmor();
				JE_drawPortConfigButtons();
			}
			else if (buttons[BUTTON_LEFT])
			{
				do
				{
					if (--pl->arc.cursor == 255)
						pl->arc.cursor = SHIPORDER_NOSECRET - 1;
					pl->items.ship = shiporder[pl->arc.cursor];
				}
				while (otherPl->player_status > STATUS_NONE && player[0].items.ship == player[1].items.ship);
				JE_playSampleNumOnChannel(S_CURSOR, SFXPRIORITY+7);
			}
			else if (buttons[BUTTON_RIGHT])
			{
				do
				{
					if (++pl->arc.cursor >= SHIPORDER_NOSECRET)
						pl->arc.cursor = 0;
					pl->items.ship = shiporder[pl->arc.cursor];					
				}
				while (otherPl->player_status > STATUS_NONE && player[0].items.ship == player[1].items.ship);
				JE_playSampleNumOnChannel(S_CURSOR, SFXPRIORITY+7);
			}

		default:
			// STATUS_INGAME, usually
			break;
	}
}


//
// Arcade HUD elements
//

void ARC_HUD_ReadyingBar( Player *this_player, JE_integer x )
{
	const JE_byte charge_time = this_player->shot_repeat[SHOT_SPECIAL];

	if (this_player->is_dragonwing)
	{
		blit_sprite2x2(VGAScreen, x + 1, 1, eShapes[5], 41);
		if (twoPlayerLinkReady)
		{
			JE_textShade(VGAScreen, x + 8, 20, "OK!", 7, this_player->hud_ready_timer > 3 ? 0 : 6, FULL_SHADE);
			++this_player->hud_ready_timer;
			this_player->hud_ready_timer %= 6;
		}
		else
			this_player->hud_ready_timer = 0;
		return;
	}

	if (this_player->items.special == 0)
		return;

	blit_sprite2x2(VGAScreen, x + 1, 1, eShapes[5], special[this_player->items.special].itemgraphic);

	if (!this_player->hud_repeat_start)
		return;

	x += 4;

	if (charge_time)
	{
		JE_byte i = 0;
		int barfill = 30 - ((charge_time * 30) / this_player->hud_repeat_start);
		JE_textShade(VGAScreen, x, 20, "-----", 7, 0, FULL_SHADE);
		while (barfill > 6 && i < 5)
		{
			barfill -= 6;
			JE_outText(VGAScreen, x + 4 * i++, 20, "-", 7, 6);
		}
		if (barfill > 0 && i < 5)
			JE_outText(VGAScreen, x + 4 * i, 20, "-", 7, barfill);

		this_player->hud_ready_timer = 0;
	}
	else if (this_player->hud_ready_timer < 18)
	{
		JE_textShade(VGAScreen, x + 4, 20, "OK!", 7, (this_player->hud_ready_timer % 6) > 3 ? 0 : 6, FULL_SHADE);
		++this_player->hud_ready_timer;
	}
}



//
// NEW MECHANICS for ALT ARCADE
//

JE_word hurryUpTimer;
JE_word hurryUpLevelLoc;

void ARC_ScoreLife( Player *this_player )
{
	if (this_player->cash >= this_player->cashForNextLife)
	{
		soundQueue[6] = S_EXPLOSION_11;
		soundQueue[7] = S_SOUL_OF_ZINGLON;

		if (this_player->lives < 11)
			++(this_player->lives);

		switch (this_player->cashForNextLife)
		{
			case 50000:	 /*   100,000 */
			case 100000: /*   200,000 */ this_player->cashForNextLife *= 2;      break;
			case 200000: /*   500,000 */ this_player->cashForNextLife = 500000;  break;
			default:     /* 1,000,000 ; 1,500,000 */
			                             this_player->cashForNextLife += 500000; break;
		}
	}
}

void ARC_Timers( void )
{
	JE_word *timer = NULL;

	// Standard level timer
	if (levelTimer && levelTimerCountdown > 0)
	{
		levelTimerCountdown--;
		if (levelTimerCountdown == 0)
			JE_eventJump(levelTimerJumpTo);
		else
			timer = &levelTimerCountdown;
	}
	else // Hurry up timer
	{    // No hurry up timer if there's already a level timer
		if (hurryUpLevelLoc < curLoc || player[0].exploding_ticks > 0 || player[0].exploding_ticks > 0)
		{
			hurryUpLevelLoc = curLoc;
			hurryUpTimer = 3000;
			if (boss_bar[0].link_num) // If fighting a boss, additional grace time
				hurryUpTimer += 1000;
		}
		else
		{
			--hurryUpTimer;
			if (hurryUpTimer == 0)
			{
				if (PL_Alive(0))
					PL_PlayerDamage(&player[0], DKILL);
				if (PL_Alive(1))
					PL_PlayerDamage(&player[1], DKILL);
			}
			else if (hurryUpTimer < 995)
				timer = &hurryUpTimer;
		}
	}

	if (!timer)
		return;

	if (*timer > 200)
	{
		if (*timer % 100 == 0)
			JE_playSampleNumOnChannel(S_WARNING, SFXPRIORITY + 1);

		if (*timer % 10 == 0)
			JE_playSampleNumOnChannel(S_CLICK, SFXPRIORITY);
	}
	else if (*timer % 20 == 0)
	{
		JE_playSampleNumOnChannel(S_WARNING, SFXPRIORITY + 1);
	}
	JE_textShade (VGAScreen, JE_fontCenter("Time Remaining", TINY_FONT) - 2, 4, "Time Remaining", 7, (*timer % 20) / 3, FULL_SHADE);
	sprintf(tmpBuf.s, "%d.%d", *timer / 100, (*timer / 10) % 10);
	JE_dString (VGAScreen, JE_fontCenter(tmpBuf.s, SMALL_FONT_SHAPES) - 2, 13, tmpBuf.s, SMALL_FONT_SHAPES);
}

void ARC_DeathSprayWeapons( Player *this_player )
{
	const uint spray = (this_player->items.weapon[0].power >= 6) ? 5 : this_player->items.weapon[0].power - 1;
	const JE_integer sys[5] = {-16, -8, -8, 0, 0};
	const JE_integer sxcs[5] = {0, -1, 1, -2, 2};
	uint i, powerItemNo;

	if (superArcadeMode == SA_NONE)
		return;

	if (normalBonusLevelCurrent || bonusLevelCurrent)
		return; // don't spray in bonus stages (they end on any death)

	// Any death that matters cuts rank
	ARC_RankCut();

	powerItemNo = 30011 + this_player->cur_weapon;

	for (i = 0; i < spray; ++i)
	{
		b = JE_newEnemy(100, 603, 0);
		if (b == 0)
			return;
		enemy[b-1].egr[1-1] = 7 + (this_player->cur_weapon * 2);
		enemy[b-1].evalue = powerItemNo;
		enemy[b-1].scoreitem = true;
		enemy[b-1].ex = this_player->x - 32;
		enemy[b-1].ey = this_player->y + sys[i];
		enemy[b-1].eyc = -5;
		enemy[b-1].eycc = 1;
		enemy[b-1].eyccadd = 1;
		enemy[b-1].eyccw = enemy[b-1].eyccwmax = 8;
		enemy[b-1].exrev = 0;
		enemy[b-1].excc = 0;
		enemy[b-1].exc = sxcs[i];
	}
}



//
// RANK
//

JE_byte currentRank = 0; // initial difficulty + (currentRank / 8)
JE_integer levelAdjustRank = 0;

static void ARC_RankSet( void )
{
	if (play_demo || record_demo)
	{
		difficultyLevel = demoDifficulty + levelAdjustRank;
		return;
	}
	difficultyLevel = DIP.startingDifficulty + levelAdjustRank + (currentRank / 8);
	if (difficultyLevel > 10)
		difficultyLevel = 10;
	if (difficultyLevel < 1)
		difficultyLevel = 1;
}

void ARC_RankIncrease( void )
{
	currentRank += DIP.rankUp;
	if ((currentRank/8 + DIP.startingDifficulty) > DIP.difficultyMax)
		currentRank = (DIP.difficultyMax - DIP.startingDifficulty) * 8 + 7;
	ARC_RankSet();
}

void ARC_RankCut( void )
{
	if (currentRank <= 8)
		currentRank = 0;
	else
		currentRank /= 2;
	ARC_RankSet();
}

void ARC_RankLevelAdjusts( JE_integer adjust )
{
	levelAdjustRank = adjust;
	ARC_RankSet();
}
