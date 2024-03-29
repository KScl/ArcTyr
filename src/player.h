/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  player.h
/// \brief Player management functions

#ifndef PLAYER_H
#define PLAYER_H

#include "config.h"
#include "opentyr.h"

#include "lib/mtrand.h"

enum
{
	ALTERNATE_SIDES = -1,
	LEFT_SIDEKICK = 0,
	RIGHT_SIDEKICK = 1
};

enum
{
	SHOT_NORMAL = 0,
	SHOT_DRAGONWING_AIMED,
	SHOT_DRAGONWING_CHARGE,
	SHOT_LEFT_SIDEKICK,
	SHOT_RIGHT_SIDEKICK,
	SHOT_MISC,
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
	BUTTON_SKICK,
	BUTTON_MODE,
	NUM_BUTTONS
};

typedef struct
{
	enum {
		STATUS_GAMEOVER = -1,
		STATUS_NONE = 0,
		STATUS_INGAME,
		// Past this point: in game, but not controlling a ship
		STATUS_SELECT,
		STATUS_NAMEENTRY,
		STATUS_CONTINUE,
	} player_status;

	struct {
		JE_word timer, timerFrac;
		JE_byte cursor;

		char hsName[10];
		Sint8 hsPos;
	} arc;

	ulong cash;
	ulong cashForNextLife;

	JE_byte shot_repeat[NUM_SHOT_TYPES];
	JE_byte shot_multi_pos[NUM_SHOT_TYPES];

	struct {
		JE_byte next_repeat;

		// flare and other repeat fire effects
		JE_byte     flare_special;
		JE_word     flare_time;
		JE_shortint flare_freq;
		JE_boolean  flare_link;
		JE_boolean  flare_spray;

		// controllable flare effects
		JE_byte     flare_control;

		// soul of zinglon
		JE_byte zinglon;
	} specials;

	struct {
		JE_byte progress[5];
		JE_byte execute;
	} twiddle;

	// option toggles
	JE_byte last_opt_given, last_opt_fired;

	// HUD
	JE_byte hud_repeat_start;
	JE_byte hud_ready_timer;

	struct
	{
		uint ship;
		uint shield;
		uint power_level;
		uint weapon[5];
		uint sidekick[2];
		uint special[2];
	} items;

	struct
	{
		uint weapon;
		uint special;
	} cur_item;
	
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

	JE_byte special_mode;
	JE_byte port_mode;
	
	int x, y;
	int initial_x, initial_y;
	int old_x[20], old_y[20];
	bool moved;
	
	int x_velocity, y_velocity;
	uint x_friction_ticks, y_friction_ticks;  // ticks until friction is applied
	
	int delta_x_shot_move, delta_y_shot_move;
	
	int last_x_shot_move, last_y_shot_move;
	int last_x_explosion_follow, last_y_explosion_follow;


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

	JE_real satRotate;
	JE_integer attachMove;
	JE_boolean attachLinked, attachReturn;
	
	JE_byte buttons[NUM_BUTTONS]; // buttons pressed / held this frame
	JE_byte last_buttons[NUM_BUTTONS]; // buttons held last frame
}
Player;

extern Player player[2];

static inline Player *PL_OtherPlayer( Player *us )
{
	return &player[us == &player[0] ? 1 : 0];
}

// Players that are in an action that has some possible gameplay relevance, including name entry, etc
static inline JE_byte PL_NumPotentialPlayers( void )
{
	return (player[0].player_status > STATUS_NONE) + (player[1].player_status > STATUS_NONE);
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

static inline JE_byte PL_RandomPlayer( void )
{
	switch (PL_WhosAlive())
	{
		case 3: return (mt_rand() % (player[0].items.power_level + player[1].items.power_level)) >= player[0].items.power_level;
		case 2: return 1;
		default: return 0;
	}
}

static inline bool all_players_dead( void )  { return (!PL_Alive(0) && !PL_Alive(1)); }

static inline bool all_players_alive( void )
{
	return ((player[0].player_status != STATUS_INGAME || player[0].is_alive)
		&&  (player[1].player_status != STATUS_INGAME || player[1].is_alive));
}

void PL_Init( Player *this_player, uint ship, bool continuing );
void PL_RandomSelect ( Player *this_player );

JE_boolean PL_ShotRepeat( Player *this_player, uint port );

void PL_SwitchWeapon( Player *this_player, uint switchTo, bool inform );
void PL_SwitchOption( Player *this_player, int side, uint switchTo, bool inform );
void PL_SwitchSpecial( Player *this_player, uint switchTo, bool inform );
bool PL_PowerUpWeapon( Player *this_player, bool inform );

JE_byte PL_PlayerDamage( Player *this_player, JE_byte damage_amt );

void PL_Twiddle( Player *this_player );

void PL_SetUpForNewLevel( void );

void PL_RandomRollSpecials( Player *this_player );
void PL_RandomRollWeapons( Player *this_player );

#endif // PLAYER_H
