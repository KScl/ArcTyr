/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcade/service.c
/// \brief Arcade service menu functions

#include "service.h"
#include "jukebox.h"

#include "../arcade.h"
#include "../config.h"
#include "../fonthand.h"
#include "../input.h"
#include "../nortsong.h"
#include "../nortvars.h"
#include "../opentyr.h"
#include "../palette.h"
#include "../params.h"
#include "../picload.h"
#include "../tyrian2.h"
#include "../varz.h"
#include "../video.h"


JE_boolean inServiceMenu = false;

//
//
//

static void (*menuNest[5])( void ) = {NULL, NULL, NULL, NULL, NULL};
static JE_byte *menuOption = NULL;
static JE_byte menuDeepness = 0;

JE_boolean ourFadeIn = false;

enum {
	__DISPLAY = 0,
	__INC,
	__DEC,
	__SELECT
} selectionType = __DISPLAY;

JE_integer numOptions, optionY;
JE_byte resetConfirm = 255;

static const char *__YesNo[] = {"No", "Yes"};

static void SRVH_DispHeader( const char *name )
{
	if (selectionType == __DISPLAY)
		JE_dString(VGAScreen, JE_fontCenter(name, FONT_SHAPES), 12, name, FONT_SHAPES);
}

static void SRVH_DispFadedOption( const char *name )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			JE_fontCenter(name, SMALL_FONT_SHAPES), optionY, 
			name, 15, -7, 
			SMALL_FONT_SHAPES, true);
		optionY += 16;
	}
}

static void SRVH_DispOption( const char *name )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			JE_fontCenter(name, SMALL_FONT_SHAPES), optionY, 
			name, 15, -4 + (*menuOption == numOptions ? 2 : 0), 
			SMALL_FONT_SHAPES, true);
		optionY += 16;
	}
}

static void SRVH_DispFadedLabel( const char *name )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			50, optionY, 
			name, 15, -7, 
			SMALL_FONT_SHAPES, true);
		optionY += 16;
	}
}

static void SRVH_DispLabel( const char *name )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			50, optionY, 
			name, 15, -4 + (*menuOption == numOptions ? 2 : 0), 
			SMALL_FONT_SHAPES, true);
		optionY += 16;
	}
}

static void SRVH_DispFadedValue( const char *value )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			270 - JE_textWidth(value, SMALL_FONT_SHAPES), optionY, 
			value, 15, -7, 
			SMALL_FONT_SHAPES, true);
	}
}

static void SRVH_DispValue( const char *value )
{
	if (selectionType == __DISPLAY)
	{
		JE_outTextAdjust(VGAScreen, 
			270 - JE_textWidth(value, SMALL_FONT_SHAPES), optionY, 
			value, 15, -4 + (*menuOption == numOptions ? 2 : 0), 
			SMALL_FONT_SHAPES, true);
	}
}

static void SRVH_Back( const char *name )
{
	if (selectionType == __SELECT && *menuOption == numOptions)
	{
		*menuOption = 0;
		menuNest[menuDeepness--] = NULL;
	}
	SRVH_DispOption(name);
	++numOptions;
}

static void SRVH_SubMenu( const char *name , void (*newFunc)( void ) )
{
	if (selectionType == __SELECT && *menuOption == numOptions)
		menuNest[++menuDeepness] = newFunc;
	SRVH_DispOption(name);
	++numOptions;
}

static void SRVH_FunctionCall( const char *name , void (*callFunc)( void ) )
{
	if (selectionType == __SELECT && *menuOption == numOptions)
		callFunc();
	SRVH_DispOption(name);
	++numOptions;
}

static void SRVH_AdjustableByte( const char *name, JE_boolean showValue, JE_byte *toAdjust, JE_byte min, JE_byte max )
{
	if (*menuOption == numOptions)
	{
		if (selectionType == __INC || selectionType == __SELECT)
		{
			if (*toAdjust >= max)
			{
				*toAdjust = max;
				if (selectionType == __INC)
					JE_playSampleNum(S_CLINK);
				else
					*toAdjust = min;
			}
			else
				++(*toAdjust);
		}
		else if (selectionType == __DEC)
		{
			if (*toAdjust <= min)
			{
				*toAdjust = min;
				JE_playSampleNum(S_CLINK);
			}
			else
				--(*toAdjust);
		}
	}
	if (showValue && selectionType == __DISPLAY)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", (JE_word)*toAdjust);
		SRVH_DispValue(tmpBuf.s);
	}
	SRVH_DispLabel(name);		
	++numOptions;
}

static void SRVH_AdjustableWord( const char *name, JE_boolean showValue, JE_word *toAdjust, JE_word min, JE_word max, JE_word incdec)
{
	if (*menuOption == numOptions)
	{
		if (selectionType == __INC || selectionType == __SELECT)
		{
			if (*toAdjust >= max - incdec)
			{
				if (*toAdjust == max)
				{
					if (selectionType == __INC)
						JE_playSampleNum(S_CLINK);
					else
						*toAdjust = min;
				}
				else
					*toAdjust = max;
			}
			else
				(*toAdjust) += incdec;
		}
		else if (selectionType == __DEC)
		{
			if (*toAdjust <= min + incdec)
			{
				if (*toAdjust == min)
					JE_playSampleNum(S_CLINK);
				*toAdjust = min;
			}
			else
				(*toAdjust) -= incdec;
		}
	}
	if (showValue && selectionType == __DISPLAY)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", *toAdjust);
		SRVH_DispValue(tmpBuf.s);
	}
	SRVH_DispLabel(name);		
	++numOptions;
}

static bool SRVH_ConfirmationAction( const char *name )
{
	if (resetConfirm == 255)
	{
		JE_playSampleNumOnChannel(V_DANGER, SFXPRIORITY+2);
		resetConfirm = 0;
	}

	SRVH_DispOption(name);

	if (selectionType != __SELECT)
	{
		SRVH_DispValue(__YesNo[resetConfirm]);
		SRVH_AdjustableByte("Are you sure?", false, &resetConfirm, 0, 1);
	}
	else
	{
		if (resetConfirm)
		{
			memcpy(&DIP, &DIP_Default, sizeof(DIP));
			JE_playSampleNumOnChannel(S_POWERUP, SFXPRIORITY+5);
		}
		SRVH_Back("");
		return (resetConfirm == 1);
	}
	return false;
}

//
// Service Menu Functions
//

void SRVF_Close( void )
{
	fade_black(35);
	JE_tyrianHalt(0);
}

void SRVF_Shutdown( void )
{
	fade_black(35);
	JE_tyrianHalt(shutdownCode);
}

void SRVF_Jukebox( void )
{
	//memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);

	fade_black(10);
	jukebox();

	// return from Jukebox
	//memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
	ourFadeIn = true;
}

//
// Service Menus
//

void SRV_ArcadeMenu( void )
{
	static const char *powerLossNames[] = {"None", "Minimal", "Minor", "Standard", "Harsh", "Harshest"};

	SRVH_DispHeader("Difficulty Settings");

	if (DIP.gameLevel == 0)
		strncpy(tmpBuf.s, "Custom", sizeof(tmpBuf.s));
	else
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "LEVEL-%d", DIP.gameLevel);
	SRVH_DispValue(tmpBuf.s);
	SRVH_AdjustableByte("Game Level", false, &DIP.gameLevel, 0, 8);

	if (selectionType != __DISPLAY)
		ArcTyr_setGameLevelSettings();

	if (DIP.gameLevel != 0)
	{
		optionY += 8;
		SRVH_DispFadedValue(difficultyNameB[DIP.startingDifficulty + 1]);
		SRVH_DispFadedLabel("Difficulty");

		SRVH_DispFadedValue(difficultyNameB[DIP.difficultyMax + 1]);
		SRVH_DispFadedLabel("Difficulty Cap");

		if (!(DIP.rankUp % 8))
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", DIP.rankUp / 8);
		else if (DIP.rankUp > 8)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu %hu/8", DIP.rankUp / 8, DIP.rankUp % 8);
		else
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu/8", DIP.rankUp);
		SRVH_DispFadedValue(tmpBuf.s);
		SRVH_DispFadedLabel("Increase Per Level");

		optionY += 8;
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", (JE_word)DIP.powerStart);
		SRVH_DispFadedValue(tmpBuf.s);
		SRVH_DispFadedLabel("Power - First Credit");

		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", (JE_word)DIP.powerContinue);
		SRVH_DispFadedValue(tmpBuf.s);
		SRVH_DispFadedLabel("Power - Continues");

		SRVH_DispFadedValue(powerLossNames[DIP.powerLoss]);
		SRVH_DispFadedLabel("Death Penalty");
	}
	else
	{
		optionY += 8;
		SRVH_DispValue(difficultyNameB[DIP.startingDifficulty + 1]);
		SRVH_AdjustableByte("Difficulty", false, &DIP.startingDifficulty, 1, 9);

		SRVH_DispValue(difficultyNameB[DIP.difficultyMax + 1]);
		SRVH_AdjustableByte("Difficulty Cap", false, &DIP.difficultyMax, DIP.startingDifficulty, 9);

		if (DIP.difficultyMax < DIP.startingDifficulty)
			DIP.difficultyMax = DIP.startingDifficulty;

		if (!(DIP.rankUp % 8))
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu", DIP.rankUp / 8);
		else if (DIP.rankUp > 8)
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu %hu/8", DIP.rankUp / 8, DIP.rankUp % 8);
		else
			snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%hu/8", DIP.rankUp);
		SRVH_DispValue(tmpBuf.s);
		SRVH_AdjustableByte("Increase Per Level", false, &DIP.rankUp, 0, 16);

		optionY += 8;
		SRVH_AdjustableByte("Power - First Credit", true, &DIP.powerStart, 1, 11);
		SRVH_AdjustableByte("Power - Continues", true, &DIP.powerContinue, 1, 11);

		SRVH_DispValue(powerLossNames[DIP.powerLoss]);
		SRVH_AdjustableByte("Death Penalty", false, &DIP.powerLoss, 0, 5);
	}

	optionY = 176;
	SRVH_Back("Back");	
}

void SRV_MoneyMenu( void )
{
	SRVH_DispHeader("Coin Settings");

	if (DIP.coinsToContinue > DIP.coinsToStart || (DIP.coinsToContinue == 0 && DIP.coinsToStart != 0))
		DIP.coinsToContinue = DIP.coinsToStart;

	if (DIP.coinsToStart == 0)
		SRVH_DispValue("Free Play");
	SRVH_AdjustableByte("Credits to Start", (DIP.coinsToStart > 0) ? true : false, &DIP.coinsToStart, 0, 8);
	if (DIP.coinsToStart == 0)
	{
		SRVH_DispFadedValue("Free Play");
		SRVH_DispFadedLabel("Credits to Continue");
	}
	else
		SRVH_AdjustableByte("Credits to Continue", true, &DIP.coinsToContinue, 1, DIP.coinsToStart);

	optionY += 16;
	SRVH_AdjustableByte("Lives - First Credit", true, &DIP.livesStart, 1, 11);
	SRVH_AdjustableByte("Lives - Continues", true, &DIP.livesContinue, 1, 11);

	optionY += 16;
	SRVH_DispValue(__YesNo[DIP.rankAffectsScore]);
	SRVH_AdjustableByte("Scale Score with Rank", false, &DIP.rankAffectsScore, 0, 1);

	optionY = 176;
	SRVH_Back("Back");	
}

void SRV_AudiovisualMenu( void )
{
	SRVH_DispHeader("Audiovisual Settings");
	if (selectionType == __DISPLAY)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%d%%", (int)(tyrMusicVolume / 2.55f));
		JE_barDrawShadow(VGAScreen, 186, optionY, 1, 174, tyrMusicVolume / 12, 3, 13);
		SRVH_DispValue(tmpBuf.s);
	}
	SRVH_AdjustableWord("Music Volume", false, &tyrMusicVolume, 0, 255, 3);

	if (selectionType == __DISPLAY)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "%d%%", (int)(fxVolume / 2.55f));
		JE_barDrawShadow(VGAScreen, 186, optionY, 1, 174, fxVolume / 12, 3, 13);
		SRVH_DispValue(tmpBuf.s);
	}
	SRVH_AdjustableWord("SFX Volume", false, &fxVolume, 0, 255, 3);

	SRVH_DispValue(detailLevel[processorType-1]);
	SRVH_AdjustableByte("Detail Level", false, &processorType, 1, 6);

	optionY += 16;

	static const char *attSound[] = {"No", "For 5 Minutes", "Yes"};
	SRVH_DispValue(attSound[DIP.attractSound]);
	SRVH_AdjustableByte("Attract Sound", false, &DIP.attractSound, 0, 2);

	if (selectionType != __DISPLAY)
	{
		JE_calcFXVol();
		set_volume(tyrMusicVolume, fxVolume);
	}

	optionY = 160;
	SRVH_FunctionCall("Sound Test / Jukebox", SRVF_Jukebox);
	SRVH_Back("Back");	
}

void SRV_ResetSettings( void )
{
	SRVH_DispHeader("Game Settings");
	SRVH_DispFadedOption("Difficulty Settings");
	SRVH_DispFadedOption("Coin Settings");
	SRVH_DispFadedOption("Audiovisual Settings");
	optionY = 128;
	if (SRVH_ConfirmationAction("Reset to Defaults"))
	{
		memcpy(&DIP, &DIP_Default, sizeof(DIP));
	}
	optionY = 176;
	SRVH_DispFadedOption("Back");
}

void SRV_Settings( void )
{
	resetConfirm = 255;

	SRVH_DispHeader("Game Settings");
	SRVH_SubMenu("Difficulty Settings", SRV_ArcadeMenu);
	SRVH_SubMenu("Coin Settings", SRV_MoneyMenu);
	SRVH_SubMenu("Audiovisual Settings", SRV_AudiovisualMenu);
	optionY = 128;
	SRVH_SubMenu("Reset to Defaults", SRV_ResetSettings);
	optionY = 176;
	SRVH_Back("Back");	
}

// ======================

JE_byte buttonsPlayerNum = 0;
JE_byte curAssignmentNum = 0;
char curAssignmentName[32] = "";

static void SRV_ButtonAssignmentSubMenu( void )
{
	const char *c;
	char *p = tmpBuf.l;
	bool lockOut = false;

	if (selectionType == __DISPLAY)
	{
		if (curAssignmentNum == 16 || curAssignmentNum == 17)
			SRVH_DispHeader("Service Buttons");
		else
		{
			sprintf(tmpBuf.l, "Player %s Buttons", buttonsPlayerNum == 0 ? "One" : "Two");
			SRVH_DispHeader(tmpBuf.l);
		}
	}

	optionY += 24;

	// Display what we're modifying and the results to it in realtime
	strcpy(tmpBuf.l, "--- No inputs ---");
	for (int i = 0; i < 4; ++i)
	{
		c = I_printableButtonCode(curAssignmentNum, i);
		if (selectionType == __DISPLAY && c[0] != '(')
		{
			if (i > 0)
			{
				*(p++) = ',';
				*(p++) = ' ';
			}
			sprintf(p, "%s", c); // strings aren't long enough to exceed tmpBuf.l[127]
			p += strlen(p);
		}

		// Actual input options (we just set them while making the big string)
		sprintf(tmpBuf.s, "Input %d", i + 1);
		if (lockOut || (i == 0 && curAssignmentNum >= INPUT_SERVICE_HOTDEBUG))
		{
			if (selectionType == __DISPLAY)
			{
				JE_outTextAdjust(VGAScreen, 270 - JE_textWidth(c, SMALL_FONT_SHAPES), optionY, c, 15, -7, 
					SMALL_FONT_SHAPES, true);
				JE_outTextAdjust(VGAScreen, 50, optionY, tmpBuf.s, 15, -7, 
					SMALL_FONT_SHAPES, true);
				optionY += 16;
			}
			continue;
		}
		else
		{
			if (selectionType == __SELECT && *menuOption == numOptions)
			{
				strcpy(tmpBuf.l, "Press any button to map it to this control.");
				JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 140, tmpBuf.l, 15, 0, FULL_SHADE);
				strcpy(tmpBuf.l, "To unset this mapping, press the button already mapped.");
				JE_textShade(VGAScreen, JE_fontCenter(tmpBuf.l, TINY_FONT), 148, tmpBuf.l, 15, 0, FULL_SHADE);
				JE_playSampleNum(I_PromptToRemapButton(curAssignmentNum, i) ? S_SELECT : S_CLINK);
				return;
			}
			SRVH_DispValue(c);
			SRVH_DispLabel(tmpBuf.s);			
		}

		++numOptions;
		if (c[0] == '(')
			lockOut = true;
	}

	if (selectionType == __DISPLAY)
	{
		optionY = 36;
		JE_textShade(VGAScreen, 280 - JE_textWidth(tmpBuf.l, TINY_FONT), optionY, tmpBuf.l, 15, 4, FULL_SHADE);
		JE_textShade(VGAScreen, 40, optionY, curAssignmentName, 15, 5, FULL_SHADE);
	}

	optionY = 176;
	SRVH_Back("Back");	
}

static void SRVH_PlayerButtonSubMenu( const char *name , void (*newFunc)( void ) , JE_byte num )
{
	if (selectionType == __SELECT && *menuOption == numOptions)
	{
		buttonsPlayerNum = num;
		menuNest[++menuDeepness] = newFunc;
	}
	SRVH_DispOption(name);
	++numOptions;
}

static void SRVH_ButtonAssignmentOption( const char *name , uint assignmentNum )
{
	if (selectionType == __DISPLAY)
	{
		const char *c;
		char *p = tmpBuf.l;

		strcpy(tmpBuf.l, "--- No inputs ---");
		for (int i = 0; i < 4; ++i)
		{
			c = I_printableButtonCode(assignmentNum, i);
			if (c[0] == '(')
				break;

			if (i > 0)
			{
				*(p++) = ',';
				*(p++) = ' ';
			}
			sprintf(p, "%s", c); // strings aren't long enough to exceed tmpBuf.l[127]
			p += strlen(p);
		}
		JE_textShade(VGAScreen, 280 - JE_textWidth(tmpBuf.l, TINY_FONT), optionY, tmpBuf.l, 15, (*menuOption == numOptions ? 4 : 0), FULL_SHADE);
		JE_textShade(VGAScreen, 40, optionY, name, 15, (*menuOption == numOptions ? 5 : 1), FULL_SHADE);
		optionY += 12;
	}
	else if (selectionType == __SELECT && *menuOption == numOptions)
	{
		curAssignmentNum = assignmentNum;
		strncpy(curAssignmentName, name, 31);
		menuNest[++menuDeepness] = SRV_ButtonAssignmentSubMenu;
	}
	++numOptions;
}

void SRV_PlayerButtons( void )
{
	sprintf(tmpBuf.l, "Player %s Buttons", buttonsPlayerNum == 0 ? "One" : "Two");
	SRVH_DispHeader(tmpBuf.l);

	SRVH_ButtonAssignmentOption("Move Up", 0 + (buttonsPlayerNum * 7));
	SRVH_ButtonAssignmentOption("Move Down", 1 + (buttonsPlayerNum * 7));
	SRVH_ButtonAssignmentOption("Move Left", 2 + (buttonsPlayerNum * 7));
	SRVH_ButtonAssignmentOption("Move Right", 3 + (buttonsPlayerNum * 7));
	optionY += 12;
	SRVH_ButtonAssignmentOption("Fire", 4 + (buttonsPlayerNum * 7));
	SRVH_ButtonAssignmentOption("Sidekick / Bomb", 5 + (buttonsPlayerNum * 7));
	SRVH_ButtonAssignmentOption("Special Mode", 6 + (buttonsPlayerNum * 7));
	optionY += 12;
	SRVH_ButtonAssignmentOption("Insert Coin", 14 + buttonsPlayerNum);
	optionY = 176;
	SRVH_Back("Back");	
}

void SRV_ServiceButtons( void )
{
	SRVH_DispHeader("Service Buttons");
	SRVH_ButtonAssignmentOption("Force Insert Coin", 17);
	optionY += 12;
	SRVH_ButtonAssignmentOption("Open In-Game Settings", 16);
	SRVH_ButtonAssignmentOption("Enter Service Menu", 18);
	optionY = 176;
	SRVH_Back("Back");	
}

void SRV_ResetInputs( void )
{
	SRVH_DispHeader("Button Config");
	SRVH_DispFadedOption("Player 1 Buttons");
	SRVH_DispFadedOption("Player 2 Buttons");
	SRVH_DispFadedOption("Service Buttons");
	optionY = 128;
	if (SRVH_ConfirmationAction("Reset to Defaults"))
	{
		I_resetConfigAssignments();
	}
	optionY = 176;
	SRVH_DispFadedOption("Back");
}


void SRV_Buttons( void )
{
	resetConfirm = 255;

	SRVH_DispHeader("Button Config");
	SRVH_PlayerButtonSubMenu("Player 1 Buttons", SRV_PlayerButtons, 0);
	SRVH_PlayerButtonSubMenu("Player 2 Buttons", SRV_PlayerButtons, 1);
	SRVH_SubMenu("Service Buttons", SRV_ServiceButtons);
	optionY = 128;
	SRVH_SubMenu("Reset to Defaults", SRV_ResetInputs);
	optionY = 176;
	SRVH_Back("Back");	
}

// ======================

void SRV_GameplayAudits( void )
{
	SRVH_DispHeader("Gameplay Audits");
	optionY = 176;
	SRVH_Back("Back");
}

void SRV_ResetAudits( void )
{
	SRVH_DispHeader("Audits");
	SRVH_DispFadedOption("Gameplay");
	SRVH_DispFadedOption("Bookkeeping");
	optionY = 128;
	if (SRVH_ConfirmationAction("Clear Audits"))
	{
		// TODO
		JE_playSampleNumOnChannel(S_POWERUP, SFXPRIORITY+5);
	}
	optionY = 176;
	SRVH_DispFadedOption("Back");
}

void SRV_Audits( void )
{
	resetConfirm = 255;

	SRVH_DispHeader("Audits");
	SRVH_Back("Gameplay");
	SRVH_Back("Bookkeeping");
	optionY = 128;
	SRVH_SubMenu("Clear Audits", SRV_ResetAudits);
	optionY = 176;
	SRVH_Back("Back");
}

// ======================

void SRV_MainMenu( void )
{
	SRVH_DispHeader("Service Menu");
	SRVH_SubMenu("Game Settings", SRV_Settings);
	SRVH_SubMenu("Button Config", SRV_Buttons);
	SRVH_SubMenu("Audits", SRV_Audits);
	optionY = 128;
	SRVH_Back("Exit Service Menu");	
	optionY = 160;
	SRVH_FunctionCall("Quit to Desktop", SRVF_Close);
	if (shutdownCode == 0)
		SRVH_DispFadedOption("Shutdown System");
	else
		SRVH_FunctionCall("Shutdown System", SRVF_Shutdown);
}

// ======================

//
//
//

void (*SRVMenuCurrent)( void ) = SRV_MainMenu;
jmp_buf service_buffer;

// Service entry point
__attribute__((noreturn)) void ARC_EnterService( void )
{
	VGAScreen = VGAScreenSeg;
	longjmp(service_buffer, 42069);
}

void ARC_Service( void )
{
	JE_byte oldMenuDeepness = 0;
	JE_byte ourOptions[5] = {0, 0, 0, 0, 0};
	uint button;

	// Well, whatever the hell we were doing, we aren't in game anymore!
	isInGame = false;
	playingCredits = false;
	inServiceMenu = true;

	skip_header_draw = true;

	play_song(SONG_MAPVIEW);
	fade_black(10);
	//JE_loadPic(VGAScreen, 2, false);

	menuDeepness = 0;
	menuNest[0] = SRV_MainMenu;
	menuOption = &ourOptions[0];

	ourFadeIn = true;
	while (menuDeepness != 0xFF)
	{
		setjasondelay(2);

		optionY = 36;
		numOptions = 0;

		if (oldMenuDeepness != menuDeepness)
		{
			oldMenuDeepness = menuDeepness;
			menuOption = &ourOptions[menuDeepness];
			//redraw = true;
		}

		JE_loadPic(VGAScreen, 2, false);

		selectionType = __DISPLAY;
		menuNest[menuDeepness]();

		JE_showVGA();
		wait_delay();

		if (ourFadeIn)
		{
			fade_palette(colors, 10, 0, 255);
			ourFadeIn = false;
		}

		button = INPUT_P1_UP;
		I_waitOnInputForMenu(button, INPUT_SERVICE_ENTER, 0);
		while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			switch (button++)
			{
			case INPUT_P1_UP:
			case INPUT_P2_UP:
				if (numOptions <= 1)
					break;
				JE_playSampleNum(S_CURSOR);
				if (--(*menuOption) == 255)
					*menuOption = numOptions - 1;
				break;
			case INPUT_SERVICE_COIN:
				if (numOptions <= 1)
				{
					// This is an assumption, but I think a valid one
					if (resetConfirm == 1)
						goto service_coin_go_left;
					goto service_coin_go_right;
				}
				// yes GCC I want to...
				// fall through
			case INPUT_P1_DOWN:
			case INPUT_P2_DOWN:
				if (numOptions <= 1)
					break;
				JE_playSampleNum(S_CURSOR);
				if (++(*menuOption) >= numOptions)
					*menuOption = 0;
				break;
			case INPUT_P1_LEFT:
			case INPUT_P2_LEFT:
			service_coin_go_left:
				JE_playSampleNum(S_CURSOR);
				selectionType = __DEC;
				numOptions = 0;
				menuNest[menuDeepness]();
				break;
			case INPUT_P1_RIGHT:
			case INPUT_P2_RIGHT:
			service_coin_go_right:
				JE_playSampleNum(S_CURSOR);
				selectionType = __INC;
				numOptions = 0;
				menuNest[menuDeepness]();
				break;
			case INPUT_P1_FIRE:
			case INPUT_P2_FIRE:
			case INPUT_SERVICE_ENTER:
				JE_playSampleNum(S_SELECT);
				selectionType = __SELECT;
				numOptions = 0;
				menuNest[menuDeepness]();
				break;
			}
		}
	}
	fade_black(10);

	skip_header_draw = false;
	inServiceMenu = false;
}
