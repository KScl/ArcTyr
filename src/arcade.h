/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
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
	JE_byte startingDifficulty;
	JE_byte rankUp;
	JE_byte difficultyMax;
	JE_byte rankAffectsScore;

	JE_byte coinsPerGame;
	JE_byte livesStart;
	JE_byte livesContinue;
	JE_byte powerStart;
	JE_byte powerContinue;

	JE_byte attractSound;

} DipSwitches;

const DipSwitches DIP_Default;
DipSwitches DIP;

extern bool attractAudioAllowed;

void ARC_IdentifyPrint( const char *s );
void ARC_IdentifyWarn( const char *s );
void ARC_Identify( void );
void ARC_IdentifyEnd( void );

void ARC_InsertCoin( void );
JE_boolean ARC_CoinStart( void );
void ARC_GetCredits( JE_word *cFull, JE_word *cPartial, JE_word *cpg );
JE_word ARC_GetCoins( void );

//

JE_boolean ARC_HS_IsLeading( uint cash );
int ARC_HS_FindPosition( uint cash );

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

#define SHIPORDER_MAX      9
#define SHIPORDER_NOSECRET 8
static const JE_byte shiporder[SHIPORDER_MAX] = {6, 7, 4, 1, 3, 5, 2, 9, 8};

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

//
//
//

#include "varz.h"

void ARC_PatcherInit( JE_byte episode, JE_byte level );
bool ARC_Patcher( struct JE_EventRecType *allEvs, JE_word *ev );
void ARC_PatcherClose( void );

#endif
