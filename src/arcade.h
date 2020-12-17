/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  arcade.h
/// \brief Arcade specific functions and data, DIP switches, etc

#ifndef ARCADE_H
#define ARCADE_H

#include "opentyr.h"
#include "player.h"

//
// DIP Switches and phyiscal hardware
//

typedef struct {
	// 0: Custom
	// 1-8: Forcibly sets the five game difficulty options to "standard" options
	JE_byte gameLevel;

	JE_byte startingDifficulty;
	JE_byte difficultyMax;
	JE_byte rankUp;
	JE_byte powerStart;
	JE_byte powerContinue;
	JE_byte powerLoss;

	JE_byte coinsToStart;
	JE_byte coinsToContinue;
	JE_byte livesStart;
	JE_byte livesContinue;
	JE_byte rankAffectsScore;

	JE_byte attractSound;

	JE_byte enableFullDebugMenus;
	JE_byte skipServiceOnStartup;
	JE_byte enableMidEpisodeStart;
} DipSwitches;

extern const DipSwitches DIP_Default;
extern DipSwitches DIP;

extern bool attractAudioAllowed;

void ARC_IdentifyPrint( const char *s );
void ARC_IdentifyWarn( const char *s );
void ARC_Identify( void );
void ARC_IdentifyEnd( void );

void ARC_InsertCoin( void );
JE_boolean ARC_CoinStart( Player *pl );
JE_word ARC_GetCoins( void );
void ARC_NextIdleScreen( void );

//

typedef struct {
	char name[10];
	uint cash, ship;
	JE_boolean isNew;
} HighScoreEntry;

HighScoreEntry highScores[20];
JE_boolean HighScore_Leading( Player *pl );
JE_boolean HighScore_InsertName( Player *pl );

//
// Visual displays
//

extern bool skip_header_draw, skip_header_undraw;
JE_byte arcTextTimer;
JE_boolean playingCredits;

void ARC_DISP_NoPlayerInSlot( uint pNum );
void ARC_DISP_InGameDisplay( uint pNum );
void ARC_DISP_HighScoreEntry( uint pNum );
void ARC_DISP_ContinueEntry( uint pNum );
void ARC_DISP_MidGameSelect( uint pNum );
void ARC_DISP_GameOverHold( uint pNum );

void ARC_SetPlayerStatus( Player *pl, int status );
void ARC_HandlePlayerStatus( Player *pl, uint pNum );

//
// HUD
//

JE_boolean twoPlayerLinkReady;

void ARC_HUD_ReadyingBar( Player *this_player, JE_integer x );


//
// Gameplay Elements
//

JE_word hurryUpTimer;
JE_word hurryUpLevelLoc;

void ARC_ScoreLife( Player *this_player );
void ARC_Timers( void );
void ARC_DeathSprayWeapons( Player *this_player );

//
// Rank
//

JE_byte currentRank;
JE_integer levelAdjustRank;

void ARC_RankIncrease( void );
void ARC_RankCut( void );
void ARC_RankLevelAdjusts( JE_integer adjust );

#endif
