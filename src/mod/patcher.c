/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  mod/patcher.c
/// \brief Level and episode patchers

#include "patcher.h"

#include "../file.h"
#include "../mainint.h"
#include "../opentyr.h"
#include "../tyrian2.h"
#include "../varz.h"

typedef struct {
	uint ev_num;
	bool append;
	struct JE_EventRecType new_ev;
} Patches;

static const Patches _P1_TYRIAN_HARD[] = {
	{5,   false, {0, 64, 6, 0, 0}}, // Remove view cone smoothie
	{17,  false, {160, 33, 533, 0, 0, 0, 0, 1}}, // restore shield powerup drop from first u-ship when fleeing
	{315, true,  {1300, 6, 17, 150, 3, 0, 0, 96}}, // add shield powerup to an oncoming enemy
	{316, false, {1300, 33, 533, 0, 0, 0, 0, 96}}, //  "
	{488, false, {2260, 100, 5, 230, 6, 0, 0, 10}}, // add normal powerup
	{508, false, {2360, 100, 5, 20, 6, 0, 0, 15}}, // add normal powerup
	{512, true,  {2400, 64, 6, 1, 1}}, // Add view cone for platforms area
	{710, false, {2845, 45, 603, 0, 0, 0, 0, 17}}, // add normal powerup to tail of a pattern
	{724, false, {2875, 45, 603, 0, 0, 0, 0, 7}}, // add normal powerup to tail of a pattern
	{858, false, {3345, 45, 603, 0, 0, 0, 0, 17}}, // add normal powerup to tail of a pattern
	{884, false, {3415, 45, 603, 0, 0, 0, 0, 17}}, // add normal powerup to tail of a pattern
	{886, true,  {3480, 64, 6, 0, 0}}, // Remove view cone for end of platforms area
	{0},
};

static const Patches _P2_GYGES[] = {
	{7,  false, {2, 33, 533, 0, 0, 0, 0, 1}}, // restore shield powerup drop from early enemy
	{0}, // End
};

static const Patches _P2_GEM_WAR[] = {
	// Gem timers: +0 399 / +105 398 / +210 397 / +315 395
	//             =x60     x165       x270       x375
	{9,   false, {60,  33, 399, 0, 0, 0, 0, 1}}, // Gem timer A
	{16,  false, {160, 101, 804, 36, 0, -6, 0, 10}}, // added shield powerup
	{17,  true,  {165, 33, 398, 0, 0, 0, 0, 1}}, // Gem timer B
	{21,  false, {190, 101, 804, 236, 0, -6, 0, 11}}, // added shield powerup
	{22,  true,  {270, 33, 397, 0, 0, 0, 0, 1}}, // Gem timer C
	{23,  true,  {350, 16, 1}}, // "Enemy approaching from behind."
	{24,  true,  {375, 33, 395, 0, 0, 0, 0, 1}}, // Gem timer D
	{31,  false, {400, 110, 809, 160, 0, 0, 0, 0}}, // added normal powerup
	{38,  false, {500, 110, 809, 220, 0, 0, 0, 0}}, // added normal powerup
	{43,  false, {560, 33, 399, 0, 0, 0, 0, 2}}, // Gem timer A
	{53,  false, {600, 110, 809, 60, 0, 0, 0, 0}}, // added normal powerup
	{54,  true,  {665, 33, 398, 0, 0, 0, 0, 2}}, // Gem timer B
	{61,  true,  {770, 33, 397, 0, 0, 0, 0, 2}}, // Gem timer C
	{64,  false, {810, 110, 809, 240, 0, 0, 0, 0}}, // added normal powerup
	{65,  false, {810, 110, 809, 50, 0, 0, 0, 0}}, // added normal powerup
	{68,  true,  {875, 33, 395, 0, 0, 0, 0, 2}}, // Gem timer D
	{103, false, {1200, 102, 805, -24, 0, 54, 0, 28}}, // added normal powerup
	{107, false, {1200, 102, 805, 276, 0, 24, 0, 27}}, // added normal powerup
	{123, false, {1400, 102, 805, -24, 0, 54, 0, 28}}, // added normal powerup
	{127, false, {1400, 102, 805, 276, 0, 24, 0, 27}}, // added normal powerup
	{155, false, {1560, 33, 399, 0, 0, 0, 0, 3}}, // Gem timer A
	{169, true,  {1665, 33, 398, 0, 0, 0, 0, 3}}, // Gem timer B
	{180, true,  {1770, 33, 397, 0, 0, 0, 0, 3}}, // Gem timer C
	{191, true,  {1875, 33, 395, 0, 0, 0, 0, 3}}, // Gem timer D
	{245, false, {2400, 102, 805, -24, 0, 54, 0, 22}}, // added normal powerup
	{249, false, {2400, 102, 805, 276, 0, 24, 0, 21}}, // added normal powerup
	{267, false, {2500, 33, 399, 0, 0, 0, 0, 4}}, // Gem timer A
	{275, true,  {2605, 33, 398, 0, 0, 0, 0, 4}}, // Gem timer B
	{284, true,  {2710, 33, 397, 0, 0, 0, 0, 4}}, // Gem timer C
	{303, true,  {2815, 33, 395, 0, 0, 0, 0, 4}}, // Gem timer D
	{344, false, {3150, 16, 1}}, // "Enemy approaching from behind." (already Danger! here, replaced)
	{347, false, {3210, 110, 809, 240, 0, 0, 0, 0}}, // added normal powerup
	{348, false, {3210, 110, 809, 50, 0, 0, 0, 0}}, // added normal powerup
	{385, false, {4050, 102, 805, -24, 0, 54, 0, 24}}, // added normal powerup
	{389, false, {4050, 102, 805, 276, 0, 24, 0, 23}}, // added normal powerup
	{405, false, {4200, 102, 805, -24, 0, 54, 0, 28}}, // added normal powerup
	{409, false, {4200, 102, 805, 276, 0, 24, 0, 27}}, // added normal powerup
	{0}, // End
};

static const Patches _P2_GRYPHON[] = {
	{63,  false, {240, 16, 1}}, // "Enemy approaching from behind."
	{64,  false, {250, 20, 0, 5, 0, 0, 0, 3}}, // Events shifted down
	{65,  false, {250, 20, 6, 0, 0, 0, 0, 4}}, // Events shifted down
	{66,  false, {270, 20, 0, 5, 0, 0, 0, 4}}, // Events shifted down
	{568, false, {2740, 16, 1}}, // "Enemy approaching from behind." (already Danger! here, replaced)
	{0},// End
};

// ---
// Episode 4
// ---

static const Patches _P4_SURFACE[] = {
	{553, false, {3220, 66, 32767, 23}}, // Skip the rolling ball of death
	{0}, // End
};

static const Patches _P4_CORE[] = {
	{661, true, {10090, 69, -1}}, // Second indefinite invuln, in case player is dead during the first one
	{0},
};

static const Patches _P4_QTUNNELQ[] = {
	{1, true, {0, 46, 3, 0, 3}}, // Greatly increase difficulty
	{716, false, {4300, 16, 8}}, // improper SFX callout changed to proper event
	{0}, // End
};

static const Patches _P4_UNDERDELI[] = {
	{525,  false, {2950, 16, 3}}, // improper SFX callout changed to proper event
	{1148, false, {9610, 62, 21}}, // improper SFX callout removed
	{0}, // End
};

static const Patches _P4_ICE_EXIT[] = {
	{524, false, {2550, 16, 1}}, // "Enemy approaching from behind." (already Danger! here, replaced)
	{0}, // End
};

static const Patches _P4_NOSE_DRIP[] = {
	{7,   true,  {5,  254, 1, 6}}, // Arcade assistance powerup
	{8,   true,  {20, 254, 2, 6}}, // Arcade assistance powerup
	{9,   true,  {35, 254, 1, 9}}, // Arcade assistance powerup
	{10,  true,  {50, 254, 2, 9}}, // Arcade assistance powerup
	{48,  false, {250, 25, 180, 0, 0, 0, 0, 13}}, // Drastically cut phase 1 health
	{173, true,  {1015, 254, 1, 6}}, // Arcade assistance powerup
	{174, true,  {1030, 254, 2, 6}}, // Arcade assistance powerup
	{187, false, {1105, 25, 180, 0, 0, 0, 0, 16}}, // Drastically cut phase 2 health
	{425, true,  {3015, 254, 1, 6}}, // Arcade assistance powerup
	{426, true,  {3030, 254, 2, 6}}, // Arcade assistance powerup
	{435, false, {3080, 25, 24, 0, 0, 0, 0, 1}}, // Drastically cut helper red thing health
	{437, false, {3199, 25, 170, 0, 0, 0, 0, 14}}, // Drastically cut phase 3 health
	{438, false, {3199, 25, 170, 0, 0, 0, 0, 15}}, // Drastically cut phase 3 health
	{511, true,  {3501, 254, 1, 6}}, // Arcade assistance powerup
	{512, true,  {3516, 254, 2, 6}}, // Arcade assistance powerup
	{514, false, {3520, 25, 24, 0, 0, 0, 0, 2}}, // Drastically cut helper red thing health
	{0}, // End
};

// ---
// Episode 4 (Tyrian 2000)
// ---

// Episode 4 slightly differs in Tyrian 2000, so we need to take this into account

static const Patches _P4T2K_CORE[] = {
	{663, true, {10090, 69, -1}}, // Second indefinite invuln, in case player is dead during the first one
	{0},
};

static const Patches _P4T2K_QTUNNELQ[] = {
	// We can't justify a difficulty increase because ?TUNNEL? isn't flagged as a bonus level in T2K
	{714, false, {4300, 16, 8}}, // improper SFX callout changed to proper event
	{0}, // End
};

static const Patches _P4T2K_UNDERDELI[] = {
	{525,  false, {2950, 16, 3}}, // improper SFX callout changed to proper event
	{1140, false, {9610, 62, 21}}, // improper SFX callout removed
	{0}, // End
};

static const Patches _P4T2K_ICE_EXIT[] = {
	{523, false, {2550, 16, 1}}, // "Enemy approaching from behind." (already Danger! here, replaced)
	{0}, // End
};

static const Patches *curPatches = NULL;

int patchSet = PATCH_DISABLED;

void MOD_PatcherSetup( const char *levelFile )
{
	FILE *f = dir_fopen_die(data_dir(), levelFile, "rb");
	unsigned int fileSize = ftell_eof(f);
	fclose(f);

	switch (fileSize)
	{
		case 538262: patchSet = PATCH_EPISODE_1; break;
		case 381242: patchSet = PATCH_EPISODE_2; break;
		case 393398: patchSet = PATCH_EPISODE_3; break;
		case 800006: patchSet = PATCH_EPISODE_4; break;
		case 538856: patchSet = PATCH_EPISODE_1_T2K; break;
		case 381187: patchSet = PATCH_EPISODE_2_T2K; break;
		case 393938: patchSet = PATCH_EPISODE_3_T2K; break;
		case 937303: patchSet = PATCH_EPISODE_4_T2K; break;
		case 521137: patchSet = PATCH_EPISODE_5_T2K; break;
		default: patchSet = PATCH_DISABLED; break;
	}

	if (patchSet >= PATCH_EPISODE_1_T2K)
		printf("patcher: Set to patch Episode %d (ver T2000)\n", patchSet - PATCH_EPISODE_4);
	else if (patchSet >= PATCH_EPISODE_1)
		printf("patcher: Set to patch Episode %d (ver 2.1)\n", patchSet);
	else
		printf("patcher: Episode not recognized, patcher disabled\n");
}

void MOD_PatcherInit( JE_byte level )
{
	curPatches = NULL;
	switch (patchSet)
	{
	default: 
		return;
	case PATCH_EPISODE_1:
	case PATCH_EPISODE_1_T2K:
		switch (level)
		{
			default: return;
			case 15: curPatches = _P1_TYRIAN_HARD; return;
		}
	case PATCH_EPISODE_2:
	case PATCH_EPISODE_2_T2K:
		switch (level)
		{
			default: return;
			case 2: curPatches = _P2_GYGES; return;
			case 6: curPatches = _P2_GRYPHON; return;
			case 7: curPatches = _P2_GEM_WAR; return;
		}
	case PATCH_EPISODE_3:
	case PATCH_EPISODE_3_T2K:
		return;
	case PATCH_EPISODE_4_T2K:
		switch (level)
		{
			default: break;
			case 5: curPatches = _P4T2K_CORE; return;
			case 7: curPatches = _P4T2K_UNDERDELI; return;
			case 8: curPatches = _P4T2K_ICE_EXIT; return;
			case 11: curPatches = _P4T2K_QTUNNELQ; return;
		}
		// fall through
	case PATCH_EPISODE_4:
		switch (level)
		{
			default: return;
			case 4: curPatches = _P4_SURFACE; return;
			case 5: curPatches = _P4_CORE; return;
			case 7: curPatches = _P4_UNDERDELI; return;
			case 8: curPatches = _P4_ICE_EXIT; return;
			case 11: curPatches = _P4_QTUNNELQ; return;
			case 16: curPatches = _P4_NOSE_DRIP; return;
		}
	case PATCH_EPISODE_5_T2K:
		return;
	}
}

bool MOD_Patcher( struct JE_EventRecType *allEvs, JE_word *ev )
{
	if (curPatches == NULL || curPatches->ev_num == 0)
		return false;

	while (*ev == curPatches->ev_num)
	{
		printf("PATCHER: patched event %d (%s)\n", *ev, curPatches->append ? "appended" : "replaced");
		memcpy(&allEvs[*ev], &curPatches->new_ev, sizeof(struct JE_EventRecType));
		if (!(curPatches++)->append)
			return true;

		// Accomodate for the newly added event
		++*ev;
		++maxEvent;
	}
	return false;
}

void MOD_PatcherClose( void )
{
	// currently a no-op
}