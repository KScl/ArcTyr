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
	this_player->items.weapon[FRONT_WEAPON].id = ships[ship].port_weapons[0];
	this_player->items.special = ships[ship].special_weapons[0];
	this_player->weapon_mode = 1;
	this_player->cur_weapon = 0;

	if (!continuing)
	{
		this_player->items.weapon[0].power = DIP.powerStart;
		this_player->lives = DIP.livesStart;
	}
	else
	{
		this_player->items.weapon[0].power = DIP.powerContinue;
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

void PL_CleanupLeavingPlayer( Player *this_player )
{
	if (this_player->player_status == STATUS_INGAME)
		return; // ???

	this_player->cash = 0;
	this_player->is_dragonwing = false;
	this_player->items.weapon[FRONT_WEAPON].id = 0;
	this_player->items.special = 0;

}

JE_boolean PL_ShotRepeat( Player *this_player, uint port )
{
	if (this_player->shot_repeat[port] == 0)
		return true;
	--this_player->shot_repeat[port];
	return false;
}


bool PL_PowerUpWeapon( Player *this_player, uint port )
{
	const bool can_power_up = this_player->items.weapon[port].id != 0 &&  // not None
	                          this_player->items.weapon[port].power < 11; // not at max power
	if (can_power_up)
	{
		++this_player->items.weapon[port].power;
		this_player->shot_multi_pos[port] = 0;
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
