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
#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"
#include "opentyr.h"

enum
{
	FRONT_WEAPON = 0,
	REAR_WEAPON = 1
};

enum
{
	LEFT_SIDEKICK = 0,
	RIGHT_SIDEKICK = 1
};

typedef struct
{
	uint ship;
	uint generator;
	uint shield;
	struct { uint id; uint power; } weapon[2];
	uint sidekick[2];
	uint special;
	
	// Dragonwing only:
	// repeatedly collecting the same powerup gives a series of sidekick upgrades
	uint sidekick_series;
	uint sidekick_level;
	
	// Single-player only
	uint super_arcade_mode;  // stored as an item for compatibility :(
}
PlayerItems;

enum
{
	SHOT_NORMAL = 0,
	SHOT_AIMED,
	SHOT_LEFT_SIDEKICK,
	SHOT_RIGHT_SIDEKICK,
	SHOT_MISC,
	SHOT_CHARGE,
	SHOT_SUPERBOMB,
	SHOT_SPECIAL,
	SHOT_SPECIAL2,
	SHOT_TWIDDLE,
	SHOT_NORTSPARKS,
	NUM_SHOT_TYPES
};

enum
{
	BUTTON_UP = 0,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_FIRE,
	BUTTON_MODE,
	BUTTON_SKICK,
	NUM_BUTTONS
};

typedef struct
{
	enum {
		STATUS_GAMEOVER = -1,
		STATUS_NONE = 0,
		STATUS_SELECT,
		STATUS_INGAME,
		STATUS_NAMEENTRY,
		STATUS_CONTINUE,
	} player_status;

	struct {
		JE_word timer, timerFrac;
		JE_byte cursor;
		char plName[10];		
	} arc;

	ulong cash;
	ulong cashForNextLife;

	JE_byte shot_repeat[NUM_SHOT_TYPES];
	JE_byte shot_multi_pos[NUM_SHOT_TYPES];

	struct {
		JE_byte next_repeat;

		// flare and other repeat fire effects
		JE_word flare_time;
		JE_shortint flare_freq;
		JE_word flare_wpn;
		JE_boolean flare_link;
		JE_boolean flare_spray;

		// soul of zinglon
		JE_byte zinglon;
	} specials;

	// option toggles
	JE_byte last_opt_given, last_opt_fired;

	// HUD
	JE_byte hud_repeat_start;
	JE_byte hud_ready_timer;

	PlayerItems items, last_items;
	
	bool is_dragonwing; // requires a bunch of special cases
	bool is_nortship; // same as above
	uint lives;
	
	// calculatable
	uint shield_max, shield_wait;
	uint initial_armor;
	uint shot_hit_area_x, shot_hit_area_y;
	
	// state
	bool is_alive;
	uint invulnerable_ticks;  // ticks until ship can be damaged
	uint exploding_ticks;     // ticks until ship done exploding
	uint shield;
	uint armor;
	uint weapon_mode;
	
	int x, y;
	int old_x[20], old_y[20];
	
	int x_velocity, y_velocity;
	uint x_friction_ticks, y_friction_ticks;  // ticks until friction is applied
	
	int delta_x_shot_move, delta_y_shot_move;
	
	int last_x_shot_move, last_y_shot_move;
	int last_x_explosion_follow, last_y_explosion_follow;

	JE_byte cur_weapon;

	struct
	{
		// calculatable
		int ammo_max;
		uint style;  // affects movement and size
		
		// state
		int x, y;
		int ammo;
		
		bool animation_enabled;
		uint animation_frame;
		
		uint charge;
		uint charge_ticks;
	}
	sidekick[2];
	
	JE_byte buttons[NUM_BUTTONS]; // buttons pressed / held this frame
	JE_byte last_buttons[NUM_BUTTONS]; // buttons held last frame
}
Player;

extern Player player[2];

static inline Player *PL_OtherPlayer( Player *player )
{
	return ((player == &player[0]) ? &player[1] : &player[0]);
}

static inline JE_byte PL_NumPlayers( void )
{
	return (player[0].player_status == STATUS_INGAME) + (player[1].player_status == STATUS_INGAME);
}

static inline JE_byte PL_WhosInGame( void )
{
	return (player[0].player_status == STATUS_INGAME) + ((player[1].player_status == STATUS_INGAME) * 2);	
}

static inline bool PL_Alive( uint pl )
{
	return (player[pl].player_status == STATUS_INGAME && player[pl].is_alive);
}

// Returns:
// 0 if nobody's alive
// 1 if Player 1 is the only one alive
// 2 if Player 2 is the only one alive
// 3 if both players are alive
static inline JE_byte PL_WhosAlive( void )
{
	return (PL_Alive(0) + (PL_Alive(1) * 2));
}

static inline bool all_players_dead( void )  { return (!PL_Alive(0) && !PL_Alive(1)); }

static inline bool all_players_alive( void )
{
	return ((player[0].player_status != STATUS_INGAME || player[0].is_alive)
		&&  (player[1].player_status != STATUS_INGAME || player[1].is_alive));
}

void PL_Init( Player *this_player, uint ship, bool continuing );

JE_boolean PL_ShotRepeat( Player *this_player, uint port );

bool power_up_weapon( Player *, uint port );

JE_byte PL_PlayerDamage( Player *this_player, JE_byte damage_amt );

#endif // PLAYER_H
