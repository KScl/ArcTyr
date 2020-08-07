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
#include "fonthand.h"
#include "lds_play.h"
#include "loudness.h"
#include "mainint.h"
#include "nortsong.h"
#include "nortvars.h"
#include "opentyr.h"
#include "playdata.h"
#include "shots.h"
#include "sprite.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

#include "lib/mtrand.h"

const JE_byte randomEnemyLaunchSounds[3] /* [1..3] */ = {13,6,26};

/*Special General Data*/
JE_byte lvlFileNum;
JE_word maxEvent, eventLoc;
/*JE_word maxenemies;*/
JE_word tempBackMove, explodeMove; /*Speed of background movement*/
JE_byte levelEnd;
JE_word deathExplodeSfxWait;
JE_shortint levelEndWarp;
JE_boolean endLevel, reallyEndLevel, waitToEndLevel, playerEndLevel,
           normalBonusLevelCurrent, bonusLevelCurrent,
           smallEnemyAdjust, readyToEndLevel;

JE_byte newPL[10]; /* [0..9] */ /*Eventsys event 75 parameter*/
JE_word returnLoc;
JE_boolean returnActive;

JE_boolean debug = false; /*Debug Mode*/
Uint32 debugTime, lastDebugTime;
JE_longint debugHistCount;
JE_real debugHist;
JE_word curLoc; /*Current Pixel location of background 1*/

JE_boolean gameNotOverYet, gameLoaded, enemyStillExploding;
uint exitGameTic, attractTic = 0;

/* Destruction Ratio */
JE_word totalEnemy;
JE_word enemyKilled[2];

/* Shape/Map Data - All in one Segment! */
struct JE_MegaDataType1 megaData1;
struct JE_MegaDataType2 megaData2;
struct JE_MegaDataType3 megaData3;

/* Secret Level Display */
JE_boolean goingToBonusLevel;
JE_byte secretLevelDisplayTime;

/* Sound Effects Queue */
JE_byte soundQueue[STANDARD_SFX_CHANNELS]; /* [0..7] */

/*Level Event Data*/
JE_boolean enemyContinualDamage;
JE_boolean enemiesActive;
JE_boolean forceEvents;
JE_boolean stopBackgrounds;
JE_byte stopBackgroundNum;
JE_byte damageRate;  /*Rate at which a player takes damage*/
JE_boolean background3x1;  /*Background 3 enemies use Background 1 X offset*/
JE_boolean background3x1b; /*Background 3 enemies moved 8 pixels left*/

JE_boolean levelTimer;
JE_word    levelTimerCountdown;
JE_word    levelTimerJumpTo;
JE_boolean randomExplosions;

JE_boolean globalFlags[10]; /* [1..10] */
JE_byte levelSong;

/* MapView Data */
JE_word mapOrigin, mapPNum;
JE_byte mapPlanet[5], mapSection[5]; /* [1..5] */

/* Enemy Data */
JE_MultiEnemyType enemy;
JE_EnemyAvailType enemyAvail;  /* values: 0: used, 1: free, 2: secret pick-up */
JE_word enemyOffset;
JE_word enemyOnScreen;
JE_byte enemyShapeTables[4];
JE_word superEnemy254Jump;

/* Enemy Shot Data*/
JE_boolean enemyShotAvail[ENEMY_SHOT_MAX]; /* [1..Enemyshotmax] */
EnemyShotType enemyShot[ENEMY_SHOT_MAX]; /* [1..Enemyshotmax]  */

/* Player Shot Data */
JE_byte     astralDuration;
JE_byte     globalFlare; // Locks other specials during use
JE_shortint globalFlareFilter;

/*PlayerData*/
JE_boolean allPlayersGone; /*Both players dead and finished exploding*/

const uint shadowYDist = 10;


JE_byte chargeWait, chargeLevel, chargeGr, chargeGrWait;

JE_word neat;


/*ExplosionData*/
explosion_type explosions[MAX_EXPLOSIONS]; /* [1..ExplosionMax] */
JE_integer explosionFollowAmountX, explosionFollowAmountY;

/*Repeating Explosions*/
rep_explosion_type rep_explosions[MAX_REPEATING_EXPLOSIONS]; /* [1..20] */

/*SuperPixels*/
superpixel_type superpixels[MAX_SUPERPIXELS]; /* [0..MaxSP] */
unsigned int last_superpixel;

/*Temporary Numbers*/
JE_byte temp, temp2, temp3;
JE_word tempX, tempY;
JE_word tempW;

JE_word x, y;
JE_integer b;

JE_byte **BKwrap1to, **BKwrap2to, **BKwrap3to,
        **BKwrap1, **BKwrap2, **BKwrap3;

void JE_getShipInfo( void )
{
	player[0].armor = ships[player[0].items.ship].dmg;
	player[1].armor = ships[player[1].items.ship].dmg;

	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		player[i].initial_armor = player[i].armor;

		if (ships[player[i].items.ship].ani == 0)
		{
			player[i].shot_hit_area_x = 12;
			player[i].shot_hit_area_y = 10;
		}
		else
		{
			player[i].shot_hit_area_x = 11;
			player[i].shot_hit_area_y = 14;
		}
	}

	// Randomizer / Mix Machine
	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		if (player[i].player_status != STATUS_INGAME)
			continue;

		if (ships[player[i].items.ship].special_weapons[0] == 0xFFFF)
		{
			do // randomize special weapon 1
				player[i].items.special[0] = (mt_rand() % num_specials) + 1;
			while (special[player[i].items.special[0]].itemgraphic == 0);

			do // randomize special weapon 2
				player[i].items.special[1] = (mt_rand() % num_specials) + 1;
			while (player[i].items.special[0] == player[i].items.special[1]
				|| special[player[i].items.special[1]].itemgraphic == 0);			
		}

		// randomize port weapons
		if (ships[player[i].items.ship].port_weapons[0] == 0xFFFF)
		{
			for (int wp = 0; wp < 5; ++wp)
			{
				retry:
				player[i].items.weapon[wp] = (mt_rand() % num_ports) + 1;
				for (int ck = wp - 1; ck >= 0; --ck)
				{
					if (player[i].items.weapon[wp] == player[i].items.weapon[ck])
						goto retry; // ensure no duplicates
				}
			}
			player[i].cur_item.weapon = player[i].items.weapon[player[i].port_mode];
			player[i].cur_item.special = player[i].items.special[player[i].special_mode];
		}
	}
}

void JE_updateOption( Player *this_player, uint i )
{
	uint p = (this_player == &player[1]) ? 1 : 0;
	JE_OptionType *this_option = &options[this_player->items.sidekick[i]];

	this_player->sidekick[i].ammo =
	this_player->sidekick[i].ammo_max = this_option->ammo;

	this_player->sidekick[i].style = this_option->tr;

	this_player->sidekick[i].animation_enabled = (this_option->option == 1);
	this_player->sidekick[i].animation_frame = 0;

	this_player->sidekick[i].charge = 0;
	this_player->sidekick[i].charge_ticks = 20;

	fill_rectangle_xy(VGAScreenSeg, hud_sidekickX[p], hud_sidekickY[i], hud_sidekickX[p] + 25, hud_sidekickY[i] + 15, 0);

	if (this_option->icongr > 0)
		blit_sprite(VGAScreenSeg, hud_sidekickX[p], hud_sidekickY[i], EXTRA_SHAPES, this_option->icongr - 1);  // sidekick HUD icon

	draw_segmented_gauge(VGAScreenSeg, 
		hud_sidekickX[p] + 1, hud_sidekickY[i] + 14, 
		112, 2, 2, MAX(1, this_player->sidekick[i].ammo_max / 8), this_player->sidekick[i].ammo);

}

void JE_updateAllOptions( void )
{
	for (uint p = 0; p < 2; ++p)
	{
		for (uint i = 0; i < COUNTOF(player[p].sidekick); ++i)
			JE_updateOption(&player[p], i);
	}
}

void JE_specialComplete( JE_byte playerNum, JE_byte specialType, uint shot_i, JE_byte twiddlePower )
{
	Player *this_player = &player[playerNum - 1];

	this_player->specials.next_repeat = 0;

	switch (special[specialType].stype)
	{
		/*Weapon*/
		case 1: // 2P OK
			b = player_shot_create(0, shot_i, this_player->x, this_player->y, special[specialType].wpn, playerNum);
			break;
		/*Repulsor*/
		case 2: // 2P OK
			for (temp = 0; temp < ENEMY_SHOT_MAX; temp++)
			{
				if (!enemyShotAvail[temp])
				{
					if (this_player->x > enemyShot[temp].sx)
						enemyShot[temp].sxm--;
					else if (this_player->x < enemyShot[temp].sx)
						enemyShot[temp].sxm++;

					if (this_player->y > enemyShot[temp].sy)
						enemyShot[temp].sym--;
					else if (this_player->y < enemyShot[temp].sy)
						enemyShot[temp].sym++;
				}
			}
			break;
		/*Zinglon Blast*/
		case 3: // 2P OK
			this_player->specials.zinglon = 50;
			this_player->shot_repeat[shot_i] = 100;
			soundQueue[7] = S_SOUL_OF_ZINGLON;
			break;
		/*Attractor*/
		case 4: // 2P OK
			for (temp = 0; temp < 125; temp++)
			{
				if (enemyAvail[temp] != 1 && enemy[temp].scoreitem
				    && enemy[temp].evalue != 0)
				{
					if (this_player->x > enemy[temp].ex)
						enemy[temp].exc++;
					else if (this_player->x < enemy[temp].ex)
						enemy[temp].exc--;

					if (this_player->y > enemy[temp].ey)
						enemy[temp].eyc++;
					else if (this_player->y < enemy[temp].ey)
						enemy[temp].eyc--;
				}
			}
			break;
		/*Flare*/
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 16:
		case 23:
		case 24:
			this_player->shot_repeat[SHOT_SPECIAL2] = 0;

			uint powerMult = this_player->items.power_level;

			this_player->specials.flare_special = specialType;
			this_player->specials.flare_freq = 8;
			this_player->specials.flare_link = false;
			this_player->specials.flare_spray = false;
			this_player->specials.flare_control = 0;
			this_player->specials.next_repeat = 200;

			globalFlare = 0;
			globalFlareFilter = -99;

			switch (special[specialType].stype)
			{
				case 5: // flare (GLOBAL)
					globalFlare = playerNum;
					globalFlareFilter = 7;
					this_player->specials.flare_freq = 2;
					this_player->specials.flare_time = 50;
					break;
				case 6: // sandstorm (GLOBAL)
					globalFlare = playerNum;
					globalFlareFilter = 1;
					this_player->specials.flare_freq = 7;
					this_player->specials.flare_time = 200 + 25 * powerMult;
					break;
				case 7: // Minefield (GLOBAL)
					globalFlare = playerNum;
					globalFlareFilter = 3;
					this_player->specials.flare_freq = 3;
					this_player->specials.flare_time = 50 + 10 * powerMult;
					this_player->specials.zinglon = 50;
					soundQueue[7] = S_SOUL_OF_ZINGLON;
					break;
				case 8: // Blade field (non global)
					this_player->specials.flare_freq = 7;
					this_player->specials.flare_time = 10 + powerMult;
					break;
				case 9: // Fire multiple (non global)
					this_player->specials.flare_time = 8 + 2 * powerMult;
					this_player->specials.flare_link = true;
					this_player->specials.next_repeat = special[specialType].pwr;
					break;
				case 10: // protron disperse (non global)
					this_player->specials.flare_time = 14 + 4 * powerMult;
					this_player->specials.flare_link = true;
					this_player->specials.next_repeat = 15;
					break;
				case 11: // nortship's astral special (GLOBAL)
					globalFlare = playerNum;
					astralDuration = 20 + 10 * powerMult;
					this_player->specials.flare_freq = special[specialType].pwr;
					this_player->specials.flare_time = 10 + 10 * powerMult;
					break;
				case 16: // mass spray microbombs everywhere like a fool (non global)
					this_player->specials.flare_time = (twiddlePower ? twiddlePower : 4) * 16 + 8;
					this_player->specials.flare_link = true;
					this_player->specials.flare_spray = true;
					break;
				case 23: // Fire multiple while sidekick held (non global)
					this_player->specials.flare_time = special[specialType].extradata;
					this_player->specials.flare_link = true;
					this_player->specials.flare_control = BUTTON_SKICK;
					break;
				case 24: // Fire multiple while fire held (non global)
					this_player->specials.flare_time = special[specialType].extradata;
					this_player->specials.flare_link = true;
					this_player->specials.flare_control = BUTTON_FIRE;
					break;
			}
			break;
		case 20: // twiddle invuln
			this_player->invulnerable_ticks = twiddlePower * 10;
			break;
		case 12: // special invuln
			b = player_shot_create(0, shot_i, this_player->x, this_player->y, special[specialType].wpn, playerNum);
			this_player->shot_repeat[shot_i] = 250;
			this_player->invulnerable_ticks = 100;
			break;
		case 13: // self heal
			this_player->armor += twiddlePower / 4 + 1;

			soundQueue[3] = S_POWERUP;
			break;

		case 22:  // spawn random sidekick with ammo
			do
				special[specialType].extradata = (mt_rand() % num_options) + 1;
			while (options[special[specialType].extradata].ammo == 0);
			// fall through
		case 19:  // spawn sidekick (alternate sides)
			soundQueue[3] = S_POWERUP;
			PL_SwitchOption(this_player, ALTERNATE_SIDES, special[specialType].extradata, true);
			if (shot_i == SHOT_SPECIAL)
				this_player->shot_repeat[SHOT_SPECIAL] = 255;
			break;

		case 21:; // spawn sidekick from weapon
			JE_byte whichOpt = 
				(this_player->items.power_level >= 8) ? 2 :
				((this_player->items.power_level >= 4) ? 1 : 0);
			JE_byte spOption = weaponPort[this_player->cur_item.weapon].dwSidekick[whichOpt];

			// Fail if no option available
			// Also, right only sidekicks fail if done on left
			if (spOption == 0 || (options[spOption].tr == 2 && special[specialType].extradata == 0))
			{
				++this_player->items.power_level;
				break;
			}

			soundQueue[3] = S_POWERUP;
			PL_SwitchOption(this_player, special[specialType].extradata, spOption, true);
			break;

		// old specials with removed behavior
		case 14: // other player heal
		case 17: // spawn left or right sidekick
		case 18: // spawn right sidekick
			break;

	}

	if (shot_i == SHOT_SPECIAL)
		this_player->hud_repeat_start = this_player->shot_repeat[SHOT_SPECIAL];
}

bool can_afford_twiddle( JE_byte playerNum, JE_byte *powerUse )
{
	Player *this_player= &player[playerNum - 1];
	bool can_afford = true;

	if (*powerUse == 0) // no cost
	{
		// No-op
	}
	else if (*powerUse < 98)  // costs some shield
	{
		if (this_player->shield >= *powerUse)
			this_player->shield -= *powerUse;
		else
			can_afford = false;
	}
	else if (*powerUse == 98)  // costs all shield
	{
		if (this_player->shield < 4)
			can_afford = false;
		*powerUse = this_player->shield;
		this_player->shield = 0;
	}
	else if (*powerUse == 99)  // costs half shield
	{
		*powerUse = this_player->shield / 2;
		this_player->shield = *powerUse;
	}
	else if (*powerUse < 200) // costs some armor
	{
		*powerUse -= 100;
		if (this_player->armor > *powerUse)
			this_player->armor -= *powerUse;
		else
			can_afford = false;
	}
	else // costs a power level
	{
		if (this_player->items.power_level <= 1)
			can_afford = false;
		else
			--this_player->items.power_level;
	}

	JE_drawShield();
	JE_drawArmor();
	return can_afford;
}

void JE_doSpecialShot( JE_byte playerNum )
{
	Player *this_player = &player[playerNum - 1];

	PL_ShotRepeat(this_player, SHOT_SPECIAL);
	PL_ShotRepeat(this_player, SHOT_TWIDDLE);

	// Can't start new twiddle or special if global flare, or local flare by this player
	// The check is present in both, instead of *surrounding* both,
	// because what if a twiddle starts a local flare?

	// Twiddles
	if (!globalFlare && this_player->specials.flare_time == 0
		&& this_player->twiddle.execute > 0 && this_player->shot_repeat[SHOT_TWIDDLE] == 0)
	{
		JE_byte twiddlePower = special[this_player->twiddle.execute].pwr;
		if (can_afford_twiddle(playerNum, &twiddlePower))
			JE_specialComplete(playerNum, this_player->twiddle.execute, SHOT_TWIDDLE, twiddlePower);
	}
	// End Twiddles

	// Main
	if (!globalFlare && this_player->specials.flare_time == 0
		&& this_player->cur_item.special > 0 && this_player->shot_repeat[SHOT_SPECIAL] == 0
		&& !this_player->last_buttons[BUTTON_FIRE] && this_player->buttons[BUTTON_FIRE])
	{
		JE_specialComplete(playerNum, this_player->cur_item.special, SHOT_SPECIAL, 0);
	}
	// End Main

	// global astral background timer
	if ((!globalFlare || playerNum == globalFlare) && astralDuration > 0)
		astralDuration--;

	if (this_player->specials.flare_control != 0)
	{
		if (!this_player->buttons[this_player->specials.flare_control])
			this_player->specials.flare_time = 1;
		else if (this_player->specials.flare_time == 1)
		{
			JE_byte dummyPower = special[this_player->specials.flare_special].pwr;
			if (can_afford_twiddle(playerNum, &dummyPower))
				this_player->specials.flare_time = special[this_player->specials.flare_special].extradata;
		}
	}

	if (this_player->specials.flare_time > 1)
	{
		if (globalFlare == playerNum && globalFlareFilter != -99)
		{
			if (levelFilter == -99 && levelBrightness == -99)
				filterActive = false;

			if (!filterActive)
			{
				filterActive = true;
				levelFilter = globalFlareFilter;
				if (levelFilter == 7)
					levelBrightness = 0;
			}

			if (levelFilter == 7)
			{
				JE_shortint flare_flicker;
				if (levelBrightness < -6)
					flare_flicker = 1;
				else if (levelBrightness > 6)
					flare_flicker = -1;
				else
					flare_flicker = ((mt_rand() & 1) << 1) - 1;
				levelBrightness += flare_flicker;
			}
		}

		if ((signed)(mt_rand() % 6) < this_player->specials.flare_freq)
		{
			b = MAX_PWEAPON;

			if (this_player->specials.flare_link)
			{
				PL_ShotRepeat(this_player, SHOT_SPECIAL2);
				if (this_player->shot_repeat[SHOT_SPECIAL2] == 0)
				{
					b = player_shot_create(0, SHOT_SPECIAL2, 
						this_player->x, this_player->y,
						special[this_player->specials.flare_special].wpn, playerNum);
				}
			}
			else
			{
				b = player_shot_create(0, SHOT_SPECIAL2, 
					mt_rand() % 280, mt_rand() % 180,
					special[this_player->specials.flare_special].wpn, playerNum);
			}

			if (this_player->specials.flare_spray && b != MAX_PWEAPON)
			{
				playerShotData[b].shotXM = (mt_rand() % 5) - 2;
				playerShotData[b].shotYM = (mt_rand() % 5) - 2;
				if (playerShotData[b].shotYM == 0)
				{
					playerShotData[b].shotYM++;
				}
			}
		}

		--this_player->specials.flare_time;
	}
	else if (this_player->specials.flare_time == 1)
	{
		this_player->specials.flare_special = 0;
		this_player->specials.flare_time = 0;
		if (this_player->specials.flare_control)
			this_player->specials.flare_control = 0;
		else
		{
			this_player->shot_repeat[SHOT_SPECIAL] = this_player->specials.next_repeat;
			this_player->hud_repeat_start = this_player->shot_repeat[SHOT_SPECIAL];
		}

		if (globalFlare == playerNum)
		{
			globalFlare = 0;
			if (levelFilter == globalFlareFilter)
			{
				levelFilter = -99;
				levelBrightness = -99;
				filterActive = false;
			}
		}
	}

	if (this_player->specials.zinglon > 0)
	{
		temp = 25 - abs(this_player->specials.zinglon - 25);

		JE_barBright(VGAScreen, this_player->x + 7 - temp,     0, this_player->x + 7 + temp,     184);
		JE_barBright(VGAScreen, this_player->x + 7 - temp - 2, 0, this_player->x + 7 + temp + 2, 184);

		--this_player->specials.zinglon;
	}
}

void JE_setupExplosion( signed int x, signed int y, signed int delta_y, unsigned int type, bool fixed_position, bool follow_player )
{
	const struct {
		JE_word sprite;
		JE_byte ttl;
	} explosion_data[53] /* [1..53] */ = {
		{ 144,  7 },
		{ 120, 12 },
		{ 190, 12 },
		{ 209, 12 },
		{ 152, 12 },
		{ 171, 12 },
		{ 133,  7 },   /*White Smoke*/
		{   1, 12 },
		{  20, 12 },
		{  39, 12 },
		{  58, 12 },
		{ 110,  3 },
		{  76,  7 },
		{  91,  3 },
/*15*/	{ 227,  3 },
		{ 230,  3 },
		{ 233,  3 },
		{ 252,  3 },
		{ 246,  3 },
/*20*/	{ 249,  3 },
		{ 265,  3 },
		{ 268,  3 },
		{ 271,  3 },
		{ 236,  3 },
/*25*/	{ 239,  3 },
		{ 242,  3 },
		{ 261,  3 },
		{ 274,  3 },
		{ 277,  3 },
/*30*/	{ 280,  3 },
		{ 299,  3 },
		{ 284,  3 },
		{ 287,  3 },
		{ 290,  3 },
/*35*/	{ 293,  3 },
		{ 165,  8 },   /*Coin Values*/
		{ 184,  8 },
		{ 203,  8 },
		{ 222,  8 },
		{ 168,  8 },
		{ 187,  8 },
		{ 206,  8 },
		{ 225, 10 },
		{ 169, 10 },
		{ 188, 10 },
		{ 207, 20 },
		{ 226, 14 },
		{ 170, 14 },
		{ 189, 14 },
		{ 208, 14 },
		{ 246, 14 },
		{ 227, 14 },
		{ 265, 14 }
	};

	if (y > -16 && y < 190)
	{
		for (int i = 0; i < MAX_EXPLOSIONS; i++)
		{
			if (explosions[i].ttl == 0)
			{
				explosions[i].x = x;
				explosions[i].y = y;
				if (type == 6)
				{
					explosions[i].y += 12;
					explosions[i].x += 2;
				} else if (type == 98)
				{
					type = 6;
				}
				explosions[i].sprite = explosion_data[type].sprite;
				explosions[i].ttl = explosion_data[type].ttl;
				explosions[i].follow_player = follow_player;
				explosions[i].fixed_position = fixed_position;
				explosions[i].delta_x = 0;
				explosions[i].delta_y = delta_y;
				break;
			}
		}
	}
}

void JE_setupExplosionLarge( JE_boolean enemyGround, JE_byte exploNum, JE_integer x, JE_integer y )
{
	if (y >= 0)
	{
		if (enemyGround)
		{
			JE_setupExplosion(x - 6, y - 14, 0,  2, false, false);
			JE_setupExplosion(x + 6, y - 14, 0,  4, false, false);
			JE_setupExplosion(x - 6, y,      0,  3, false, false);
			JE_setupExplosion(x + 6, y,      0,  5, false, false);
		} else {
			JE_setupExplosion(x - 6, y - 14, 0,  7, false, false);
			JE_setupExplosion(x + 6, y - 14, 0,  9, false, false);
			JE_setupExplosion(x - 6, y,      0,  8, false, false);
			JE_setupExplosion(x + 6, y,      0, 10, false, false);
		}

		bool big;

		if (exploNum > 10)
		{
			exploNum -= 10;
			big = true;
		}
		else
		{
			big = false;
		}

		if (exploNum)
		{
			for (int i = 0; i < MAX_REPEATING_EXPLOSIONS; i++)
			{
				if (rep_explosions[i].ttl == 0)
				{
					rep_explosions[i].ttl = exploNum;
					rep_explosions[i].delay = 2;
					rep_explosions[i].x = x;
					rep_explosions[i].y = y;
					rep_explosions[i].big = big;
					break;
				}
			}
		}
	}
}

void JE_drawShield( void )
{
	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		if (player[i].player_status != STATUS_INGAME)
			continue;

		fill_rectangle_xy(VGAScreenSeg, (i == 0) ? 3 : 295, 137 - 15, (i == 0) ? 11 : 303, 194 - 15, 0);
		JE_dBar3(VGAScreenSeg, (i == 0) ? 3 : 295, 194 - 15, player[i].shield, 144);

		if (player[i].shield != player[i].shield_max)
		{
			const uint y = (193 - 15) - (player[i].shield_max * 2);
			JE_rectangle(VGAScreenSeg, (i == 0) ? 3 : 295, y, (i == 0) ? 11 : 303, y, 68);
		}
	}
}

void JE_drawArmor( void )
{
	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		if (player[i].player_status != STATUS_INGAME)
			continue;
		if (player[i].armor > 28)
			player[i].armor = 28;

		fill_rectangle_xy(VGAScreenSeg, (i == 0) ? 16 : 308, 137 - 15, (i == 0) ? 24 : 316, 194 - 15, 0);

		// Arcade extra: Flash the player's armor bar
		if (player[i].is_alive && player[i].armor < 6)
		{
			fill_rectangle_xy(VGAScreenSeg, (i == 0) ? 16 : 308, 137 - 15, (i == 0) ? 18 : 310, 194 - 15, warningCol);
			fill_rectangle_xy(VGAScreenSeg, (i == 0) ? 22 : 314, 137 - 15, (i == 0) ? 24 : 316, 194 - 15, warningCol);
		}

		JE_dBar3(VGAScreenSeg, (i == 0) ? 16 : 308, 194 - 15, player[i].armor, 224);
	}
}

void JE_doSP( JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color ) /* superpixels */
{
	for (temp = 0; temp < num; temp++)
	{
		JE_real tempr = mt_rand_lt1() * (2 * M_PI);
		signed int tempy = roundf(cosf(tempr) * mt_rand_1() * explowidth);
		signed int tempx = roundf(sinf(tempr) * mt_rand_1() * explowidth);

		if (++last_superpixel >= MAX_SUPERPIXELS)
			last_superpixel = 0;
		superpixels[last_superpixel].x = tempx + x;
		superpixels[last_superpixel].y = tempy + y;
		superpixels[last_superpixel].delta_x = tempx;
		superpixels[last_superpixel].delta_y = tempy + 1;
		superpixels[last_superpixel].color = color;
		superpixels[last_superpixel].z = 15;
	}
}

void JE_drawSP( void )
{
	for (int i = MAX_SUPERPIXELS; i--; )
	{
		if (superpixels[i].z)
		{
			superpixels[i].x += superpixels[i].delta_x;
			superpixels[i].y += superpixels[i].delta_y;

			if (superpixels[i].x < (unsigned)VGAScreen->w && superpixels[i].y < (unsigned)VGAScreen->h)
			{
				Uint8 *s = (Uint8 *)VGAScreen->pixels; /* screen pointer, 8-bit specific */
				s += superpixels[i].y * VGAScreen->pitch;
				s += superpixels[i].x;

				*s = (((*s & 0x0f) + superpixels[i].z) >> 1) + superpixels[i].color;
				if (superpixels[i].x > 0)
					*(s - 1) = (((*(s - 1) & 0x0f) + (superpixels[i].z >> 1)) >> 1) + superpixels[i].color;
				if (superpixels[i].x < VGAScreen->w - 1u)
					*(s + 1) = (((*(s + 1) & 0x0f) + (superpixels[i].z >> 1)) >> 1) + superpixels[i].color;
				if (superpixels[i].y > 0)
					*(s - VGAScreen->pitch) = (((*(s - VGAScreen->pitch) & 0x0f) + (superpixels[i].z >> 1)) >> 1) + superpixels[i].color;
				if (superpixels[i].y < VGAScreen->h - 1u)
					*(s + VGAScreen->pitch) = (((*(s + VGAScreen->pitch) & 0x0f) + (superpixels[i].z >> 1)) >> 1) + superpixels[i].color;
			}

			superpixels[i].z--;
		}
	}
}

