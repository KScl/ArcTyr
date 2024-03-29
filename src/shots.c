/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  shots.c
/// \brief Player shot management

#include "config.h"
#include "playdata.h"
#include "player.h"
#include "shots.h"
#include "sprite.h"
#include "video.h"
#include "varz.h"

// I'm pretty sure the last extra entry was never used.
PlayerShotDataType playerShotData[MAX_PWEAPON]; /* [1..MaxPWeapon+1] */
JE_byte shotAvail[MAX_PWEAPON]; /* [1..MaxPWeapon] */   /*0:Avail 1-255:Duration left*/


void simulate_player_shots( void )
{
	// Player Shot Images
	for (int z = 0; z < MAX_PWEAPON; z++)
	{
		if (shotAvail[z] != 0)
		{
			shotAvail[z]--;
			if (z != MAX_PWEAPON - 1)
			{
				PlayerShotDataType* shot = &playerShotData[z];

				if (--shot->shotXCW <= 0)
				{
					shot->shotXM += shot->shotXC;
					shot->shotXCW = shot->shotXCWmax;
				}
				if (shot->shotXM <= 100)
					shot->shotX += shot->shotXM;

				if (--shot->shotYCW <= 0)
				{
					shot->shotYM += shot->shotYC;
					shot->shotYCW = shot->shotYCWmax;
				}
				shot->shotY += shot->shotYM;

				if (shot->shotYM > 100)
				{
					shot->shotY -= 120;
					shot->shotY += player[0].delta_y_shot_move;
				}

				if (shot->shotComplicated != 0)
				{
					shot->shotDevX += shot->shotDirX;
					shot->shotX += shot->shotDevX;

					if (abs(shot->shotDevX) == shot->shotCirSizeX)
						shot->shotDirX = -shot->shotDirX;

					shot->shotDevY += shot->shotDirY;
					shot->shotY += shot->shotDevY;

					if (abs(shot->shotDevY) == shot->shotCirSizeY)
						shot->shotDirY = -shot->shotDirY;
					// Double Speed Circle Shots - add a second copy of above loop
				}

				int tempShotX = shot->shotX;
				int tempShotY = shot->shotY;

				if (shot->shotX < 0 || shot->shotX > 140 ||
				    shot->shotY < 0 || shot->shotY > 170)
				{
					shotAvail[z] = 0;
					goto draw_player_shot_loop_end;
				}

//				if (shot->shotTrail != 255)
//				{
//					if (shot->shotTrail == 98)
//					{
//						JE_setupExplosion(shot->shotX - shot->shotXM, shot->shotY - shot->shotYM, shot->shotTrail);
//					} else {
//						JE_setupExplosion(shot->shotX, shot->shotY, shot->shotTrail);
//					}
//				}

				JE_word anim_frame = shot->shotGr + shot->shotAni;
				if (++shot->shotAni == shot->shotAniMax)
					shot->shotAni = 0;

				if (anim_frame < 60000)
				{
					if (anim_frame > 1000)
						anim_frame = anim_frame % 1000;
					if (anim_frame > 500)
						blit_sprite2(VGAScreen, tempShotX+1, tempShotY, shotShapes[1], anim_frame - 500);
					else
						blit_sprite2(VGAScreen, tempShotX+1, tempShotY, shotShapes[0], anim_frame);
				}
			}

draw_player_shot_loop_end:
			;
		}
	}
}

static void player_shot_set_direction( JE_integer shot_id, JE_real direction )
{
	static const JE_word aimedGrs[][17] = {
	//   DOWN            RIGHT           UP              LEFT            DOWN
		{ 77,221,183,301,  1,282,164,202, 58,201,163,281, 39,300,182,220, 77},
		{ 85,242,131,303, 47,284,150,223, 66,224,149,283,  9,302,130,243, 85},
		{ 78,299,295,297,  2,278,276,280, 59,279,275,277, 40,296,294,298, 78},
		{307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,322,323},
		{326,327,328,329,330,331,332,333,334,335,336,337,338,339,340,341,342},
		{364,365,366,367,368,369,370,371,372,373,374,375,376,377,378,379,380},
		{383,384,385,386,387,388,389,390,391,392,393,394,395,396,397,398,399},
	};

	PlayerShotDataType* shot = &playerShotData[shot_id];

	// Some weapons have sprites for each direction, use those.
	int rounded_dir = roundf(direction * (16 / (2 * M_PI)));
	for (int i = 0; i < 5; ++i)
	{
		if (shot->shotGr == aimedGrs[i][8])
		{
			shot->shotGr = aimedGrs[i][rounded_dir];
			return;
		}
	}
}

bool player_shot_move_and_draw(
		int shot_id, bool* out_is_special,
		int* out_shotx, int* out_shoty,
		JE_word* out_special_radiusw, JE_word* out_special_radiush )
{
	PlayerShotDataType* shot = &playerShotData[shot_id];

	shotAvail[shot_id]--;

	if (--shot->shotXCW <= 0)
	{
		shot->shotXM += shot->shotXC;
		shot->shotXCW = shot->shotXCWmax;
	}
	shot->shotX += shot->shotXM;
	JE_integer tmp_shotXM = shot->shotXM;

	if (shot->shotXM > 100)
	{
		if (shot->shotXM == 101)
		{
			shot->shotX -= 101;
			shot->shotX += player[shot->playerNumber-1].delta_x_shot_move;
			shot->shotY += player[shot->playerNumber-1].delta_y_shot_move;
		}
		else
		{
			shot->shotX -= 120;
			shot->shotX += player[shot->playerNumber-1].delta_x_shot_move;
		}
	}

	if (--shot->shotYCW <= 0)
	{
		shot->shotYM += shot->shotYC;
		shot->shotYCW = shot->shotYCWmax;
	}
	shot->shotY += shot->shotYM;

	if (shot->shotYM > 100)
	{
		shot->shotY -= 120;
		shot->shotY += player[shot->playerNumber-1].delta_y_shot_move;
	}

	if (shot->shotComplicated != 0)
	{
		shot->shotDevX += shot->shotDirX;
		shot->shotX += shot->shotDevX;

		if (abs(shot->shotDevX) == shot->shotCirSizeX)
			shot->shotDirX = -shot->shotDirX;

		shot->shotDevY += shot->shotDirY;
		shot->shotY += shot->shotDevY;

		if (abs(shot->shotDevY) == shot->shotCirSizeY)
			shot->shotDirY = -shot->shotDirY;

		/*Double Speed Circle Shots - add a second copy of above loop*/
	}

	*out_shotx = shot->shotX;
	*out_shoty = shot->shotY;

	if (shot->shotX < -34 || shot->shotX > 290 ||
		shot->shotY < -15 || shot->shotY > 190)
	{
		shotAvail[shot_id] = 0;
		return false;
	}

	switch (shot->shotTrail)
	{
		case 255: // no trail
			break;
		case 98:
			JE_setupExplosion(shot->shotX - shot->shotXM, shot->shotY - shot->shotYM, 0, shot->shotTrail, false, false);
			break;
		default:
			JE_setupExplosion(shot->shotX, shot->shotY, 0, shot->shotTrail, false, false);
			break;
	}

	if (shot->aimAtEnemy != 0)
	{
		if (--shot->aimDelay == 0)
		{
			shot->aimDelay = shot->aimDelayMax;

			if (enemyAvail[shot->aimAtEnemy - 1] != 1)
			{
				if (shot->shotX < enemy[shot->aimAtEnemy - 1].ex)
					shot->shotXM++;
				else
					shot->shotXM--;

				if (shot->shotY < enemy[shot->aimAtEnemy - 1].ey)
					shot->shotYM++;
				else
					shot->shotYM--;
			}
			else
			{
				if (shot->shotXM > 0)
					shot->shotXM++;
				else
					shot->shotXM--;
			}
		}
	}

	JE_word sprite_frame = shot->shotGr + shot->shotAni;
	if (++shot->shotAni == shot->shotAniMax)
		shot->shotAni = 0;

	*out_is_special = sprite_frame > 60000;

	if (*out_is_special)
	{
		blit_sprite_blend(VGAScreen, *out_shotx+1, *out_shoty, OPTION_SHAPES, sprite_frame - 60001);

		*out_special_radiusw = sprite(OPTION_SHAPES, sprite_frame - 60001)->width / 2;
		*out_special_radiush = sprite(OPTION_SHAPES, sprite_frame - 60001)->height / 2;
	}
	else
	{
		if (sprite_frame > 1000)
		{
			JE_doSP(*out_shotx+1 + 6, *out_shoty + 6, 5, 3, (sprite_frame / 1000) << 4);
			sprite_frame = sprite_frame % 1000;
		}
		if (sprite_frame > 500)
		{
			if (background2 && *out_shoty + shadowYDist < 190 && tmp_shotXM < 100)
				blit_sprite2_darken(VGAScreen, *out_shotx+1, *out_shoty + shadowYDist, shotShapes[1], sprite_frame - 500);
			blit_sprite2(VGAScreen, *out_shotx+1, *out_shoty, shotShapes[1], sprite_frame - 500);
		}
		else
		{
			if (background2 && *out_shoty + shadowYDist < 190 && tmp_shotXM < 100)
				blit_sprite2_darken(VGAScreen, *out_shotx+1, *out_shoty + shadowYDist, shotShapes[0], sprite_frame);
			blit_sprite2(VGAScreen, *out_shotx+1, *out_shoty, shotShapes[0], sprite_frame);
		}
	}

	return true;
}

JE_integer player_shot_create( JE_word portNum, uint bay_i, JE_word PX, JE_word PY, JE_word wpNum, JE_byte playerNum )
{
	static const JE_byte soundChannel[2][NUM_SHOT_TYPES] = 
		{{0, 0, 1, 4, 4, 4, 1, 1, 1, 4},
		 {2, 2, 1, 4, 4, 4, 1, 1, 1, 4}};

	// Bounds check
	if (portNum > num_ports || wpNum <= 0 || wpNum > num_pWeapons)
		return MAX_PWEAPON;

	const JE_WeaponType* weapon = &pWeapons[wpNum];

	if (weapon->sound > 0)
		soundQueue[soundChannel[playerNum - 1][bay_i]] = weapon->sound;

	int shot_id = MAX_PWEAPON;
	Player *this_player = &player[playerNum - 1];
	JE_byte *sMP = &this_player->shot_multi_pos[bay_i];

	/*Rot*/
	for (int multi_i = 1; multi_i <= weapon->multi; multi_i++)
	{
		for (shot_id = 0; shot_id < MAX_PWEAPON; shot_id++)
			if (shotAvail[shot_id] == 0)
				break;
		if (shot_id == MAX_PWEAPON)
		{
			printf("player_shot_create: no empty slots for piece %hu:%hhu\n", wpNum, *sMP);
			return MAX_PWEAPON;
		}

		if (*sMP == weapon->max || *sMP > 8)
			*sMP = 1;
		else
			(*sMP)++;

		PlayerShotDataType* shot = &playerShotData[shot_id];

		shot->playerNumber = playerNum;

		shot->shotAni = 0;

		shot->shotComplicated = weapon->circlesize[*sMP-1] != 0;

		if (weapon->circlesize[*sMP-1] == 0)
		{
			shot->shotDevX = 0;
			shot->shotDirX = 0;
			shot->shotDevY = 0;
			shot->shotDirY = 0;
			shot->shotCirSizeX = 0;
			shot->shotCirSizeY = 0;
		}
		else
		{
			JE_byte circsize = weapon->circlesize[*sMP-1];

			if (circsize > 240) // Left Down
			{
				circsize -= 240;
				shot->shotDirX = -1;
				shot->shotDirY = 1;
			}
			else if (circsize > 220) // Left Up
			{
				circsize -= 220;
				shot->shotDirX = shot->shotDirY = -1;
			}
			else if (circsize > 200) // Right Down
			{
				circsize -= 200;
				shot->shotDirX = shot->shotDirY = 1;
			}
			else // Default direction: Right Up
			{
				shot->shotDirX = 1;
				shot->shotDirY = -1;
			}

			if (circsize > 19)
			{
				JE_byte circsize_mod20 = circsize % 20;
				shot->shotCirSizeX = circsize_mod20;
				shot->shotDevX = circsize_mod20 >> 1;

				circsize = circsize / 20;
				shot->shotCirSizeY = circsize;
				shot->shotDevY = circsize >> 1;
			}
			else
			{
				shot->shotCirSizeX = circsize;
				shot->shotCirSizeY = circsize;
				shot->shotDevX = circsize >> 1;
				shot->shotDevY = circsize >> 1;
			}
			shot->shotDevX *= shot->shotDirX;
			shot->shotDevY *= -shot->shotDirY;
		}

		shot->shotTrail = weapon->trail;

		shot->shotDmg = weapon->dmgAmount[*sMP-1];
		shot->shrapnel = weapon->dmgChain[*sMP-1];
		shot->ice = weapon->dmgIce[*sMP-1];
		shot->infinite = weapon->dmgInfinite[*sMP-1];

		shot->shotBlastFilter = weapon->shipblastfilter;

		JE_integer tmp_by = weapon->by[*sMP-1];

		/*Note: Only front selection used for player shots...*/

		shot->shotX = PX + weapon->bx[*sMP-1];

		shot->shotY = PY + tmp_by;

		// Shot acceleration hacks
		if (abs(weapon->acceleration[*sMP-1]) > 16)
		{
			shot->shotYC  = weapon->acceleration[*sMP-1] > 0 ? -1 : 1;
			shot->shotYCW = shot->shotYCWmax = abs(weapon->acceleration[*sMP-1]) - 16;
		}
		else
		{
			shot->shotYC = -weapon->acceleration[*sMP-1];
			shot->shotYCW = shot->shotYCWmax = 0;
		}

		if (abs(weapon->accelerationx[*sMP-1]) > 16)
		{
			shot->shotXC  = weapon->accelerationx[*sMP-1] > 0 ? 1 : -1;
			shot->shotXCW = shot->shotXCWmax = abs(weapon->accelerationx[*sMP-1]) - 16;
		}
		else
		{
			shot->shotXC = weapon->accelerationx[*sMP-1];
			shot->shotXCW = shot->shotXCWmax = 0;
		}
		// End shot acceleration hacks

		shot->shotXM = weapon->sx[*sMP-1];

		// Not sure what this field does exactly.
		JE_byte del = weapon->del[*sMP-1];

		if (del == 121)
		{
			shot->shotTrail = 0;
			del = 255;
		}

		shot->shotGr = weapon->sg[*sMP-1];
		if (shot->shotGr == 0)
			shotAvail[shot_id] = 0;
		else
			shotAvail[shot_id] = del;

		if (del > 100 && del < 120)
			shot->shotAniMax = (del - 100 + 1);
		else
			shot->shotAniMax = weapon->weapani + 1;

		if (del == 99 || del == 98)
		{
			tmp_by = PX - this_player->initial_x;
			if (tmp_by < -5)
				tmp_by = -5;
			else if (tmp_by > 5)
				tmp_by = 5;
			shot->shotXM += tmp_by;
		}

		if (del == 99 || del == 100)
		{
			tmp_by = PY - this_player->initial_y - weapon->sy[*sMP-1];
			if (tmp_by < -4)
				tmp_by = -4;
			else if (tmp_by > 4)
				tmp_by = 4;
			shot->shotYM = tmp_by;
		}
		else if (weapon->sy[*sMP-1] == 98)
		{
			shot->shotYM = 0;
			shot->shotYC = -1;
		}
		else if (weapon->sy[*sMP-1] > 100)
		{
			shot->shotYM = weapon->sy[*sMP-1];
			shot->shotY -= this_player->delta_y_shot_move;
		}
		else
		{
			shot->shotYM = -weapon->sy[*sMP-1];
		}

		if (weapon->sx[*sMP-1] > 100)
		{
			shot->shotXM = weapon->sx[*sMP-1];
			shot->shotX -= this_player->delta_x_shot_move;
			if (shot->shotXM == 101)
				shot->shotY -= this_player->delta_y_shot_move;
		}


		if (weapon->aim > 5)  /*Guided Shot*/
		{
			uint best_dist = 65000;
			JE_byte closest_enemy = 0;
			/*Find Closest Enemy*/
			for (x = 0; x < 100; x++)
			{
				if (enemyAvail[x] != 1 && !enemy[x].scoreitem)
				{
					y = abs(enemy[x].ex - shot->shotX) + abs(enemy[x].ey - shot->shotY);
					if (y < best_dist)
					{
						best_dist = y;
						closest_enemy = x + 1;
					}
				}
			}
			shot->aimAtEnemy = closest_enemy;
			shot->aimDelay = 5;
			shot->aimDelayMax = weapon->aim - 5;
		}
		else
		{
			shot->aimAtEnemy = 0;
		}

		this_player->shot_repeat[bay_i] = weapon->shotrepeat;

		if (bay_i == SHOT_DRAGONWING_AIMED)
		{
			shot->shotX = PX + roundf(
				(weapon->bx[*sMP-1] +  1.0f) * cosf(-linkGunDirec) +
				(weapon->by[*sMP-1] + 20.0f) * sinf(linkGunDirec)
			);
			shot->shotY = PY + roundf(
				(weapon->bx[*sMP-1] +  1.0f) * sinf(-linkGunDirec) +
				(weapon->by[*sMP-1] + 20.0f) * cosf(linkGunDirec)
			);

			shot->shotXM = (int)roundf(
				(cosf(-linkGunDirec) * weapon->sx[*sMP-1]) +
				(sinf(linkGunDirec)  * weapon->sy[*sMP-1])
			);
			shot->shotYM = (int)roundf(
				(sinf(-linkGunDirec) * weapon->sx[*sMP-1]) +
				(cosf(linkGunDirec)  * weapon->sy[*sMP-1])
			);

			player_shot_set_direction(shot_id, linkGunDirec);
		}
	}

	return shot_id;
}
