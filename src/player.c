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
#include "episodes.h"
#include "mainint.h"
#include "nortsong.h"
#include "player.h"
#include "sndmast.h"
#include "varz.h"

Player player[2];

void PL_Init( Player *this_player, uint ship, bool continuing )
{
	// unrolled
	this_player->items.weapon[0] = ships[ship].port_weapons[0];
	this_player->items.weapon[1] = ships[ship].port_weapons[1];
	this_player->items.weapon[2] = ships[ship].port_weapons[2];
	this_player->items.weapon[3] = ships[ship].port_weapons[3];
	this_player->items.weapon[4] = ships[ship].port_weapons[4];
	this_player->port_mode = 0;
	this_player->items.special[0] = ships[ship].special_weapons[0];
	this_player->items.special[1] = ships[ship].special_weapons[1];
	this_player->special_mode = 0;

	this_player->cur_item.weapon = this_player->items.weapon[0];
	this_player->cur_item.special = this_player->items.special[0];

	if (!continuing)
	{
		this_player->items.power_level = DIP.powerStart;
		this_player->lives = DIP.livesStart;
	}
	else
	{
		this_player->items.power_level = DIP.powerContinue;
		this_player->lives = DIP.livesContinue;		
	}

	this_player->items.sidekick[0] = 0;
	this_player->items.sidekick[1] = 0;

	switch (ships[ship].shipgraphic)
	{
	case 0: // Dragonwing
		this_player->is_dragonwing = true;
		this_player->is_nortship = false;
		this_player->items.sidekick[0] = 30;  // Satellite Marlo
		break;
	case 1: // Nortship
		this_player->is_dragonwing = false;
		this_player->is_nortship = true;
		this_player->items.sidekick[0] = 24;  // Companion Ship Quicksilver
		this_player->items.sidekick[1] = 24;  // Companion Ship Quicksilver
		break;
	default:
		this_player->is_dragonwing = false;
		this_player->is_nortship = false;
		break;
	}

	this_player->player_status = STATUS_INGAME;
	this_player->cash = 0;
	this_player->cashForNextLife = 50000;
}

// Unused?
void PL_CleanupLeavingPlayer( Player *this_player )
{
	if (this_player->player_status == STATUS_INGAME)
		return; // ???

	this_player->cash = 0;
	this_player->is_dragonwing = false;
	this_player->cur_item.weapon = 0;
	this_player->cur_item.special = 0;
}

JE_boolean PL_ShotRepeat( Player *this_player, uint port )
{
	if (this_player->shot_repeat[port] == 0)
		return true;
	--this_player->shot_repeat[port];
	return false;
}

void PL_SwitchWeapon( Player *this_player, uint switchTo, bool inform )
{
	this_player->port_mode = switchTo;
	this_player->cur_item.weapon = this_player->items.weapon[switchTo];

	this_player->shot_multi_pos[SHOT_NORMAL] = 0;
	this_player->shot_repeat[SHOT_NORMAL] = 10;

	if (inform)
	{
		char *color = JE_textColorFromPWeapon(switchTo);
		char *wName = JE_trim(weaponPort[this_player->cur_item.weapon].name);

		if (PL_NumPlayers() == 2)
			sprintf(tmpBuf.s, "Player %d got", this_player == &player[0] ? 1 : 2);
		else
			sprintf(tmpBuf.s, "You got");

		if (!strncmp(wName, "The", 3))
			sprintf(tmpBuf.l, "%s %s%s", tmpBuf.s, color, wName);
		else
			sprintf(tmpBuf.l, "%s the %s%s", tmpBuf.s, color, wName);
		JE_drawTextWindowColorful(tmpBuf.l);
	}
}

void PL_SwitchSpecial( Player *this_player, uint switchTo, bool inform )
{
	this_player->special_mode = switchTo;
	this_player->cur_item.special = this_player->items.special[switchTo];

	this_player->shot_multi_pos[SHOT_SPECIAL] = 0;
	this_player->shot_multi_pos[SHOT_SPECIAL2] = 0;

	if (inform)
	{
		char *wName = JE_trim(special[this_player->cur_item.special].name);

		if (PL_NumPlayers() == 2)
			sprintf(tmpBuf.s, "Player %d got", this_player == &player[0] ? 1 : 2);
		else
			sprintf(tmpBuf.s, "You got");

		if (!strncmp(wName, "The", 3))
			sprintf(tmpBuf.l, "%s %s%s", tmpBuf.s, "^04", wName);
		else
			sprintf(tmpBuf.l, "%s the %s%s", tmpBuf.s, "^04", wName);
		JE_drawTextWindowColorful(tmpBuf.l);
	}
}

bool PL_PowerUpWeapon( Player *this_player, bool inform )
{
	bool can_power_up = (this_player->items.power_level < 11);

	this_player->shot_multi_pos[SHOT_NORMAL] = 0;
	this_player->shot_repeat[SHOT_NORMAL] = 10;

	if (can_power_up)
		++this_player->items.power_level;

	if (inform)
	{
		char *color = JE_textColorFromPWeapon(this_player->port_mode);
		char *wName = JE_trim(weaponPort[this_player->cur_item.weapon].name);		

		if (PL_NumPlayers() == 2)
			sprintf(tmpBuf.s, "Player %d's ", this_player == &player[0] ? 1 : 2);
		else
			tmpBuf.s[0] = 0;

		if (can_power_up)
			sprintf(tmpBuf.l, "%s%s%s ^04%s", tmpBuf.s, color, wName, "powered up");
		else
			sprintf(tmpBuf.l, "%s%s%s ^04%s", tmpBuf.s, color, wName, "power is maxed!");
		JE_drawTextWindowColorful(tmpBuf.l);
	}

	return can_power_up;
}


JE_byte PL_PlayerDamage( Player *this_player, JE_byte damage_amt )
{
	bool die = false, shieldBurst = false;
	int playerDamage = 0;

	soundQueue[7] = S_SHIELD_HIT;

	if (!this_player->is_alive)
		return 0; // sanity check??

	// Instant kill (used for hurryup time up)
	if (damage_amt == DKILL)
		die = true;

	// Dragonwing linked; share damage
	else if (twoPlayerLinked)
	{
		if (this_player->is_dragonwing)
			return 0; // The dragonwing half should be invulnerable

		int total_shield = player[0].shield + player[1].shield;
		if (total_shield < damage_amt)
		{
			playerDamage = damage_amt;
			damage_amt -= total_shield;
			player[0].shield = player[1].shield = 0;

			// through shields -- now armor
			// if (damage_amt > 0) is never false
			int total_armor = player[0].armor + player[1].armor;
			if (total_armor < damage_amt)
				die = true;
			else if (total_armor != 0)
			{
				float armor_ratio = (float)player[0].armor / total_armor;
				JE_byte admg_proportion = roundf(armor_ratio * (float)damage_amt);

				damage_amt -= admg_proportion;
				player[0].armor -= admg_proportion;
				player[1].armor -= damage_amt;

				soundQueue[7] = S_HULL_HIT;
			}
		}
		else if (total_shield != 0)
		{
			float shield_ratio = (float)player[0].shield / total_shield;
			JE_byte sdmg_proportion = roundf(shield_ratio * (float)damage_amt);

			damage_amt -= sdmg_proportion;
			player[0].shield -= sdmg_proportion;
			player[1].shield -= damage_amt;

			shieldBurst = true;
		}
	}

	// Player Damage Routines
	else 
	{
		if (this_player->shield < damage_amt)
		{
			playerDamage = damage_amt;
			damage_amt -= this_player->shield;
			this_player->shield = 0;

			// through shields -- now armor
			// if (damage_amt > 0) is never false
			if (this_player->armor < damage_amt)
				die = true;
			else
			{
				this_player->armor -= damage_amt;
				soundQueue[7] = S_HULL_HIT;
			}
		}
		else
		{
			this_player->shield -= damage_amt;
			shieldBurst = true;
		}
	}

	if (die && !youAreCheating)
	{
		if (twoPlayerLinked)
		{
			player[0].shield          = player[1].shield          = 0;
			player[0].armor           = player[1].armor           = 0;
			player[0].is_alive        = player[1].is_alive        = false;
			player[0].exploding_ticks = player[1].exploding_ticks = 60;
			ARC_DeathSprayWeapons(&player[0]);
			ARC_DeathSprayWeapons(&player[1]);
		}
		else
		{
			this_player->shield = 0;
			this_player->armor = 0;
			this_player->is_alive = false;
			this_player->exploding_ticks = 60;
			ARC_DeathSprayWeapons(this_player);
		}
		twoPlayerLinked = false;
		levelEnd = 40;
		tempVolume = tyrMusicVolume;
		soundQueue[1] = S_EXPLOSION_22;
	}
	else if (shieldBurst)
	{
		bool twoP = (PL_NumPlayers() == 2);

		JE_setupExplosion(this_player->x - 17, this_player->y - 12, 0, 14, false, !twoP);
		JE_setupExplosion(this_player->x - 5 , this_player->y - 12, 0, 15, false, !twoP);
		JE_setupExplosion(this_player->x + 7 , this_player->y - 12, 0, 16, false, !twoP);
		JE_setupExplosion(this_player->x + 19, this_player->y - 12, 0, 17, false, !twoP);

		JE_setupExplosion(this_player->x - 17, this_player->y + 2, 0,  18, false, !twoP);
		JE_setupExplosion(this_player->x + 19, this_player->y + 2, 0,  19, false, !twoP);

		JE_setupExplosion(this_player->x - 17, this_player->y + 16, 0, 20, false, !twoP);
		JE_setupExplosion(this_player->x - 5 , this_player->y + 16, 0, 21, false, !twoP);
		JE_setupExplosion(this_player->x + 7 , this_player->y + 16, 0, 22, false, !twoP);
	}

	JE_drawShield();
	JE_drawArmor();

	return playerDamage;
}

void PL_Twiddle( Player *this_player )
{
	JE_byte twidDir = 99, twidCheckDirs;
	JE_byte currentKey;
	JE_byte i;

	JE_ShipType *ship = &ships[this_player->items.ship];

	this_player->twiddle.execute = 0;

	// this uses buttons instead of PX/Y and mouseX/Y
	// possibly more precise than before?
	twidCheckDirs = (this_player->buttons[BUTTON_UP])   +  /*UP*/
	                (this_player->buttons[BUTTON_DOWN]) +  /*DOWN*/
	                (this_player->buttons[BUTTON_LEFT]) +  /*LEFT*/
	                (this_player->buttons[BUTTON_RIGHT]);  /*RIGHT*/

	if (twidCheckDirs == 0) // no direction being pressed
	{
		// only if all buttons released
		if (this_player->buttons[BUTTON_FIRE] || this_player->buttons[BUTTON_SKICK])
			return;
	}
	else if (twidCheckDirs == 1)
	{
		twidDir = (this_player->buttons[BUTTON_UP])    * 1 + /*UP*/
		          (this_player->buttons[BUTTON_DOWN])  * 2 + /*DOWN*/
		          (this_player->buttons[BUTTON_LEFT])  * 3 + /*LEFT*/
		          (this_player->buttons[BUTTON_RIGHT]) * 4 + /*RIGHT*/
		          (this_player->buttons[BUTTON_FIRE])  * 4 +
		          (this_player->buttons[BUTTON_SKICK]) * 8;
	}
	else // more than one direction pressed
		return;

	// 1: UP      5: UP+FIRE      9: UP+SKICK
	// 2: DOWN    6: DOWN+FIRE   10: DOWN+SKICK
	// 3: LEFT    7: LEFT+FIRE   11: LEFT+SKICK
	// 4: RIGHT   8: RIGHT+FIRE  12: RIGHT+SKICK
	// 99: All buttons and directions released

	for (i = 0; i < ship->numTwiddles; i++)
	{
		// get next combo key
		currentKey = ship->twiddles[i][this_player->twiddle.progress[i]];

		// correct key
		if (currentKey == twidDir)
		{
			// if next key is a special number, code is done
			currentKey = ship->twiddles[i][++this_player->twiddle.progress[i]];
			if (currentKey > 100)
			{
				this_player->twiddle.progress[i] = 0;
				this_player->twiddle.execute = currentKey - 100;
			}
		}
		else if (this_player->twiddle.progress[i])
		{
			if ((twidDir != 99) && // Don't reset if waiting for all release
			    ((currentKey - 1) & 3) != ((twidDir - 1) & 3) && // Don't reset if correct direction but wrong buttons
			    (ship->twiddles[i][this_player->twiddle.progress[i]-1] != twidDir)) // Don't reset if previous direction is our current
			{
				this_player->twiddle.progress[i] = 0;
			}
		}
	}
}
