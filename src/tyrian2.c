/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  tyrian2.c
/// \brief TODO

#include "animlib.h"
#include "arcade.h"
#include "backgrnd.h"
#include "episodes.h"
#include "file.h"
#include "font.h"
#include "fonthand.h"
#include "input.h"
#include "lds_play.h"
#include "loudness.h"
#include "menus.h"
#include "mainint.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"
#include "pcxload.h"
#include "picload.h"
#include "playdata.h"
#include "shots.h"
#include "sprite.h"
#include "tyrian2.h"
#include "vga256d.h"
#include "video.h"

#include "lib/mtrand.h"

#include "dev/demo.h"

#include "mod/patcher.h"

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

inline static void blit_enemy( SDL_Surface *surface, unsigned int i, signed int x_offset, signed int y_offset, signed int sprite_offset );

boss_bar_t boss_bar[2];

/* Level Event Data */
JE_boolean quit;

struct JE_EventRecType eventRec[EVENT_MAXIMUM]; /* [1..eventMaximum] */
JE_word levelEnemyMax;
JE_word levelEnemyFrequency;
JE_word levelEnemy[40]; /* [1..40] */

char tempStr[31];

// formerly in musmast.c
static JE_boolean musicFade;

static void init_saweapon_bag( void )
{
	JE_byte tmp_b;
	int rnd = mt_rand();

	for (int i = 0; i < 5; ++i)
		SAPowerupBag[i] = i + 1;

	if ((rnd = (mt_rand() % 5)) != 0)
	{
		tmp_b = SAPowerupBag[0];
		SAPowerupBag[0] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if ((rnd = 1 + (mt_rand() & 3)) != 1)
	{
		tmp_b = SAPowerupBag[1];
		SAPowerupBag[1] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if ((rnd = 2 + (mt_rand() % 3)) != 2)
	{
		tmp_b = SAPowerupBag[2];
		SAPowerupBag[2] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if (mt_rand() & 1)
	{
		tmp_b = SAPowerupBag[3];
		SAPowerupBag[3] = SAPowerupBag[4];
		SAPowerupBag[4] = tmp_b;
	}

	superArcadePowerUp = 0;
}

static void randomize_saweapon_bag( void )
{
	JE_byte tmp_b;
	int rnd = mt_rand();

	// note: for first iteration, can never swap last to first,
	// so no double powerups

	// This is technically an unrolled loop, I guess?
	if ((rnd = (mt_rand() & 3)) != 0)
	{
		tmp_b = SAPowerupBag[0];
		SAPowerupBag[0] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if ((rnd = 1 + (mt_rand() & 3)) != 1)
	{
		tmp_b = SAPowerupBag[1];
		SAPowerupBag[1] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if ((rnd = 2 + (mt_rand() % 3)) != 2)
	{
		tmp_b = SAPowerupBag[2];
		SAPowerupBag[2] = SAPowerupBag[rnd];
		SAPowerupBag[rnd] = tmp_b;
	}
	if (mt_rand() & 1)
	{
		tmp_b = SAPowerupBag[3];
		SAPowerupBag[3] = SAPowerupBag[4];
		SAPowerupBag[4] = tmp_b;
	}

	superArcadePowerUp = 0;
}

static inline bool can_play_audio( void )
{
	if (play_demo)
		return attractAudioAllowed;
	return gameNotOverYet;
}

void JE_copyGameToVGA( void )
{
	JE_byte *src;
	Uint8 *s = NULL; /* screen pointer, 8-bit specific */

	int x, y;

	if (!playerEndLevel)
	{
		s = VGAScreenSeg->pixels;
		s += 28;

		src = game_screen->pixels;
		src += 24;

		if (smoothScroll != 0 /*&& thisPlayerNum != 2*/)
		{
			wait_delay();
			setjasondelay(frameCountMax);
		}

		if (starShowVGASpecialCode == 1)
		{
			src += game_screen->pitch * 183;
			for (y = 0; y < 184; y++)
			{
				memmove(s, src, 264);
				s += VGAScreenSeg->pitch;
				src -= game_screen->pitch;
			}
		}
		else if (starShowVGASpecialCode == 2 && processorType >= 2)
		{
			int lighty = 172 - player[0].y;
			int lightx = 281 - player[0].x;
			int lighty2 = 172 - player[1].y;
			int lightx2 = 281 - player[1].x;
			int lightdist, lightdist2;

			if (player[0].player_status != STATUS_INGAME)
				lighty = 200;
			if (player[1].player_status != STATUS_INGAME)
				lighty2 = 200;

			int minY = (lighty2 < lighty) ? lighty2 : lighty;
			for (y = 184; y >= minY; --y)
			{
				for (x = 320 - 56; x; --x)
				{
					lightdist = abs(lightx - x) + lighty;
					lightdist2 = abs(lightx2 - x) + lighty2;
					if (lightdist2 < lightdist)
						lightdist = lightdist2;

					if (lightdist < y)
						*s = *src;
					else if (lightdist - y <= 5)
						*s = (*src & 0xf0) | (((*src & 0x0f) + (3 * (5 - (lightdist - y)))) / 4);
					else
						*s = (*src & 0xf0) | ((*src & 0x0f) >> 2);

					s++;
					src++;
				}
				s += 56 + VGAScreenSeg->pitch - 320;
				src += 56 + VGAScreenSeg->pitch - 320;
			}
			for (; y; --y)
			{
				for (x = 320 - 56; x; --x)
				{
					*s = (*src & 0xf0) | ((*src >> 2) & 0x03);
					s++;
					src++;
				}
				s += 56 + VGAScreenSeg->pitch - 320;
				src += 56 + VGAScreenSeg->pitch - 320;
			}
		}
		else
		{
			for (y = 0; y < 184; y++)
			{
				memmove(s, src, 264);
				s += VGAScreenSeg->pitch;
				src += game_screen->pitch;
			}
		}
	}
}

inline static void blit_enemy( SDL_Surface *surface, unsigned int i, signed int x_offset, signed int y_offset, signed int sprite_offset )
{
	if (enemy[i].sprite2s <= (Sprite2_array *)1)
	{
		// error just once -- FRUIT is a clusterfuck
		if (enemy[i].sprite2s == NULL)
			fprintf(stderr, "warning: enemy %d sprite missing\n", i);
		enemy[i].sprite2s = (Sprite2_array *)1;
		return;
	}
	
	const int x = enemy[i].ex + x_offset + tempMapXOfs,
	          y = enemy[i].ey + y_offset;
	const unsigned int index = enemy[i].egr[enemy[i].enemycycle - 1] + sprite_offset;

	if (enemy[i].filter != 0)
		blit_sprite2_filter(surface, x, y, *enemy[i].sprite2s, index, enemy[i].filter);
	else
		blit_sprite2(surface, x, y, *enemy[i].sprite2s, index);
}

void JE_drawEnemy( int enemyOffset ) // actually does a whole lot more than just drawing
{
	JE_integer enX, enY;
	int target_p;

	player[0].x -= 25;
	player[1].x -= 25;

	for (int i = enemyOffset - 25; i < enemyOffset; i++)
	{
		if (enemyAvail[i] != 1)
		{
			enemy[i].mapoffset = tempMapXOfs;

			if (enemy[i].xaccel && enemy[i].xaccel - 89u > mt_rand() % 11)
			{
				if (player[enemy[i].playertotarget].x > enemy[i].ex)
				{
					if (enemy[i].exc < enemy[i].xaccel - 89)
						enemy[i].exc++;
				}
				else
				{
					if (enemy[i].exc >= 0 || -enemy[i].exc < enemy[i].xaccel - 89)
						enemy[i].exc--;
				}
			}

			if (enemy[i].yaccel && enemy[i].yaccel - 89u > mt_rand() % 11)
			{
				if (player[enemy[i].playertotarget].y > enemy[i].ey)
				{
					if (enemy[i].eyc < enemy[i].yaccel - 89)
						enemy[i].eyc++;
				}
				else
				{
					if (enemy[i].eyc >= 0 || -enemy[i].eyc < enemy[i].yaccel - 89)
						enemy[i].eyc--;
				}
			}

 			if (enemy[i].ex + tempMapXOfs > -29 && enemy[i].ex + tempMapXOfs < 300)
			{
				if (enemy[i].aniactive == 1)
				{
					enemy[i].enemycycle++;

					if (enemy[i].enemycycle == enemy[i].animax)
						enemy[i].aniactive = enemy[i].aniwhenfire;
					else if (enemy[i].enemycycle > enemy[i].ani)
						enemy[i].enemycycle = enemy[i].animin;
				}

				if (enemy[i].egr[enemy[i].enemycycle - 1] == 999)
					goto enemy_gone;

				if (enemy[i].size == 1) // 2x2 enemy
				{
					if (enemy[i].ey > -13)
					{
						blit_enemy(VGAScreen, i, -6, -7, 0);
						blit_enemy(VGAScreen, i,  6, -7, 1);
					}
					if (enemy[i].ey > -26 && enemy[i].ey < 182)
					{
						blit_enemy(VGAScreen, i, -6,  7, 19);
						blit_enemy(VGAScreen, i,  6,  7, 20);
					}
				}
				else
				{
					if (enemy[i].ey > -13)
						blit_enemy(VGAScreen, i, 0, 0, 0);
				}

				enemy[i].filter = 0;
			}

			if (enemy[i].excc)
			{
				if (--enemy[i].exccw <= 0)
				{
					if (enemy[i].exc == enemy[i].exrev)
					{
						enemy[i].excc = -enemy[i].excc;
						enemy[i].exrev = -enemy[i].exrev;
						enemy[i].exccadd = -enemy[i].exccadd;
					}
					else
					{
						enemy[i].exc += enemy[i].exccadd;
						enemy[i].exccw = enemy[i].exccwmax;
						if (enemy[i].exc == enemy[i].exrev)
						{
							enemy[i].excc = -enemy[i].excc;
							enemy[i].exrev = -enemy[i].exrev;
							enemy[i].exccadd = -enemy[i].exccadd;
						}
					}
				}
			}

			if (enemy[i].eycc)
			{
				if (--enemy[i].eyccw <= 0)
				{
					if (enemy[i].eyc == enemy[i].eyrev)
					{
						enemy[i].eycc = -enemy[i].eycc;
						enemy[i].eyrev = -enemy[i].eyrev;
						enemy[i].eyccadd = -enemy[i].eyccadd;
					}
					else
					{
						enemy[i].eyc += enemy[i].eyccadd;
						enemy[i].eyccw = enemy[i].eyccwmax;
						if (enemy[i].eyc == enemy[i].eyrev)
						{
							enemy[i].eycc = -enemy[i].eycc;
							enemy[i].eyrev = -enemy[i].eyrev;
							enemy[i].eyccadd = -enemy[i].eyccadd;
						}
					}
				}
			}

			enemy[i].ey += enemy[i].fixedmovey;

			enemy[i].ex += enemy[i].exc;
			if (enemy[i].ex < -80 || enemy[i].ex > 340)
				goto enemy_gone;

			enemy[i].ey += enemy[i].eyc;
			if (enemy[i].ey < -112 || enemy[i].ey > 190)
				goto enemy_gone;

			if (i >= 100 && enemy[i].eycc)
			{
				if (enemy[i].ey < 0)
					enemy[i].ey = 0;
				if (enemy[i].eyc >= 1)
				{
					enemy[i].eycc = 0;
					enemy[i].excc = 4;
					enemy[i].exrev = 4;
					enemy[i].exccadd = 1;
					enemy[i].exccw = 1;
				}
			}

			goto enemy_still_exists;

enemy_gone:
			/* enemy[i].egr[10] &= 0x00ff; <MXD> madness? */
			enemyAvail[i] = 1;
			goto draw_enemy_end;

enemy_still_exists:

			/*X bounce*/
			if (enemy[i].ex <= enemy[i].xminbounce || enemy[i].ex >= enemy[i].xmaxbounce)
				enemy[i].exc = -enemy[i].exc;

			/*Y bounce*/
			if (enemy[i].ey <= enemy[i].yminbounce || enemy[i].ey >= enemy[i].ymaxbounce)
				enemy[i].eyc = -enemy[i].eyc;

			/* Evalue != 0 - score item at boundary */
			if (enemy[i].scoreitem)
			{
				if (enemy[i].ex < -5)
					enemy[i].ex++;
				if (enemy[i].ex > 245)
					enemy[i].ex--;
			}

			enemy[i].ey += tempBackMove;

			if (enemy[i].ex <= -24 || enemy[i].ex >= 296)
				goto draw_enemy_end;

			enX = enemy[i].ex;
			enY = enemy[i].ey;

			temp = enemy[i].enemytype;

			/* Enemy Shots */
			if (enemy[i].edamaged == 1)
				goto draw_enemy_end;

			enemyOnScreen++;

			if (enemy[i].iced)
			{
				enemy[i].iced--;
				if (enemy[i].enemyground != 0)
				{
					enemy[i].filter = 0x09;
				}
				goto draw_enemy_end;
			}

			for (int j = 3; j > 0; j--)
			{
				if (enemy[i].freq[j-1])
				{
					temp3 = enemy[i].tur[j-1];

					if (--enemy[i].eshotwait[j-1] == 0 && temp3)
					{
						switch (difficultyLevel)
						{
							default: enemy[i].eshotwait[j-1] = enemy[i].freq[j-1]; break;
							case 3: enemy[i].eshotwait[j-1] = (enemy[i].freq[j-1] / 2) + 1; break;
							case 4:
							case 5: enemy[i].eshotwait[j-1] = (enemy[i].freq[j-1] / 2); break;
							case 6:
							case 7: enemy[i].eshotwait[j-1] = (enemy[i].freq[j-1] * 2) / 3; break;
							case 8:
							case 9: enemy[i].eshotwait[j-1] = (enemy[i].freq[j-1] / 3); break;
							case 10: enemy[i].eshotwait[j-1] = (enemy[i].freq[j-1] / 4); break;
						}
						if (enemy[i].eshotwait[j-1] == 0)
							enemy[i].eshotwait[j-1] = 1;

						// formerly galaga:
						//if (galagaMode && (enemy[i].eyc == 0 || (mt_rand() % 400) >= galagaRandomShotChance))
						//	goto draw_enemy_end;

						switch (temp3)
						{
						case 252: /* Savara Boss DualMissile */
							if (enemy[i].ey > 20)
							{
								JE_setupExplosion(enX - 8 + tempMapXOfs, enY - 20 - backMove * 8, -2, 6, false, false);
								JE_setupExplosion(enX + 4 + tempMapXOfs, enY - 20 - backMove * 8, -2, 6, false, false);
							}
							break;
						case 251:; /* Suck-O-Magnet */
							int attractivity;
							for (int p = 0; p < 2; ++p)
							{
								if (PL_Alive(p))
								{
									attractivity = 4 - (abs(player[p].x - enX) + abs(player[p].y - enY)) / 100;
									player[p].x_velocity += (player[p].x > enX) ? -attractivity : attractivity;
								}
							} 
							break;
						case 253: /* Left ShortRange Magnet */
							for (int p = 0; p < 2; ++p)
							{
								if (PL_Alive(p) && abs(player[p].x + 25 - 14 - enX) < 24 && abs(player[p].y - enY) < 28)
									player[p].x_velocity += 2;
							} 
							break;
						case 254: /* Left ShortRange Magnet */
							for (int p = 0; p < 2; ++p)
							{
								if (PL_Alive(p) && abs(player[p].x + 25 - 14 - enX) < 24 && abs(player[p].y - enY) < 28)
									player[p].x_velocity -= 2;
							} 
							break;
						case 255: /* Magneto RePulse!! */
							if (difficultyLevel != 1) /*DIF*/
							{
								if (j == 3)
								{
									enemy[i].filter = 0x70;
								}
								else
								{
									int repulsivity;
									for (int p = 0; p < 2; ++p)
									{
										if (PL_Alive(p))
										{
											repulsivity = 4 - (abs(player[p].x - enX) + abs(player[p].y - enY)) / 20;
											if (repulsivity > 0)
												player[p].x_velocity += (player[p].x > enX) ? repulsivity : -repulsivity;
										}
									}
								}
							}
							break;
						default:
							// all bullets fired in one round all aim for the same target
							target_p = PL_RandomPlayer();

						/*Rot*/
							const JE_EnemyWeaponType *ewpn = &eWeapons[temp3];
							for (int tempCount = ewpn->multi; tempCount > 0; tempCount--)
							{
								for (b = 0; b < ENEMY_SHOT_MAX; b++)
								{
									if (enemyShotAvail[b] == 1)
										break;
								}
								if (b == ENEMY_SHOT_MAX)
									goto draw_enemy_end;

								enemyShotAvail[b] = !enemyShotAvail[b];

								if (ewpn->sound > 0)
								{
									do
										temp = mt_rand() % 8;
									while (temp == 3);
									soundQueue[temp] = ewpn->sound;
								}

								if (enemy[i].aniactive == 2)
									enemy[i].aniactive = 1;

								if (++enemy[i].eshotmultipos[j-1] > ewpn->max)
									enemy[i].eshotmultipos[j-1] = 1;

								int tempPos = enemy[i].eshotmultipos[j-1] - 1;

								if (j == 1)
									temp2 = 4;

								enemyShot[b].sx = enX + ewpn->bx[tempPos] + tempMapXOfs;
								enemyShot[b].sy = enY + ewpn->by[tempPos];
								enemyShot[b].sdmg = ewpn->attack[tempPos];
								enemyShot[b].tx = ewpn->tx;
								enemyShot[b].ty = ewpn->ty;
								enemyShot[b].duration = ewpn->del[tempPos];
								enemyShot[b].animate = 0;
								enemyShot[b].animax = ewpn->weapani;

								enemyShot[b].sgr = ewpn->sg[tempPos];
								switch (j)
								{
								case 1:
									enemyShot[b].syc = ewpn->acceleration[tempPos];
									enemyShot[b].sxc = ewpn->accelerationx[tempPos];

									enemyShot[b].sxm = ewpn->sx[tempPos];
									enemyShot[b].sym = ewpn->sy[tempPos];
									break;
								case 3:
									enemyShot[b].sxc = -ewpn->acceleration[tempPos];
									enemyShot[b].syc = ewpn->accelerationx[tempPos];

									enemyShot[b].sxm = -ewpn->sy[tempPos];
									enemyShot[b].sym = -ewpn->sx[tempPos];
									break;
								case 2:
									enemyShot[b].sxc = ewpn->acceleration[tempPos];
									enemyShot[b].syc = -ewpn->acceleration[tempPos];

									enemyShot[b].sxm = ewpn->sy[tempPos];
									enemyShot[b].sym = -ewpn->sx[tempPos];
									break;
								}

								if (ewpn->aim > 0)
								{
									int aim = ewpn->aim;

									/*DIF*/
									switch (difficultyLevel)
									{
										default: break;
										case 3: aim += 1; break;
										case 4: /*  += 2 */
										case 5: aim += 2; break;
										case 6: aim += 3; break;
										case 7: aim += 4; break;
										case 8: aim += 5; break;
										case 9: /*  +  6 */
										case 10: aim += 6; break;
									}

									JE_word target_x = player[target_p].x;
									JE_word target_y = player[target_p].y;

									int relative_x = (target_x + 25) - enX - tempMapXOfs - 4;
									if (relative_x == 0)
										relative_x = 1;
									int relative_y = target_y - enY;
									if (relative_y == 0)
										relative_y = 1;
									const int longest_side = MAX(abs(relative_x), abs(relative_y));
									enemyShot[b].sxm = roundf((float)relative_x / longest_side * aim);
									enemyShot[b].sym = roundf((float)relative_y / longest_side * aim);
								}
							}
							break;
						}
					}
				}
			}

			/* Enemy Launch Routine */
			if (enemy[i].launchfreq)
			{
				if (--enemy[i].launchwait == 0)
				{
					enemy[i].launchwait = enemy[i].launchfreq;

					if (enemy[i].launchspecial != 0)
					{
						/*Type  1 : Must be inline with player*/
						if (PL_Alive(0) && abs(enemy[i].ey - player[0].y) <= 5)
							enemy[i].playertotarget = 0;
						else if (PL_Alive(1) && abs(enemy[i].ey - player[1].y) <= 5)
							enemy[i].playertotarget = 1;
						else
							goto draw_enemy_end;
					}

					if (enemy[i].aniactive == 2)
					{
						enemy[i].aniactive = 1;
					}

					if (enemy[i].launchtype == 0)
						goto draw_enemy_end;

					tempW = enemy[i].launchtype;
					b = JE_newEnemy(enemyOffset == 50 ? 75 : enemyOffset - 25, tempW, 0);

					/*Launch Enemy Placement*/
					if (b > 0)
					{
						struct JE_SingleEnemyType* e = &enemy[b-1];

						e->ex = enX;
						e->ey = enY + enemyDat[e->enemytype].startyc;
						if (e->size == 0)
							e->ey -= 7;

						if (e->launchtype > 0 && e->launchfreq == 0)
						{
							if (e->launchtype > 90)
							{
								e->ex += mt_rand() % ((e->launchtype - 90) * 4) - (e->launchtype - 90) * 2;
							}
							else
							{
								int target_x = (player[e->playertotarget].x + 25) - enX - tempMapXOfs - 4;
								if (target_x == 0)
									target_x = 1;
								int tempI5 = player[e->playertotarget].y - enY;
								if (tempI5 == 0)
									tempI5 = 1;
								const int longest_side = MAX(abs(target_x), abs(tempI5));
								e->exc = roundf(((float)target_x / longest_side) * e->launchtype);
								e->eyc = roundf(((float)tempI5 / longest_side) * e->launchtype);
							}
						}

						do
							temp = mt_rand() % 8;
						while (temp == 3);
						soundQueue[temp] = randomEnemyLaunchSounds[(mt_rand() % 3)];

						if (enemy[i].launchspecial == 1
						    && enemy[i].linknum < 100)
						{
							e->linknum = enemy[i].linknum;
						}
					}
				}
			}
		}
draw_enemy_end:
		;
	}

	player[0].x += 25;
	player[1].x += 25;
}

void JE_main( void )
{
	char buffer[256];

	int lastEnemyOnScreen;

	/* NOTE: BEGIN MAIN PROGRAM HERE AFTER LOADING A GAME OR STARTING A NEW ONE */

	/* ----------- GAME ROUTINES ------------------------------------- */
	/* We need to jump to the beginning to make space for the routines */
	/* --------------------------------------------------------------- */
	goto start_level_first;


	/*------------------------------GAME LOOP-----------------------------------*/


	/* Startlevel is called after a previous level is over.  If the first level
	   is started for a gaming session, startlevelfirst is called instead and
	   this code is skipped.  The code here finishes the level and prepares for
	   the loadmap function. */

start_level:

	free_sprite2s(&eShapes[0]);
	free_sprite2s(&eShapes[1]);
	free_sprite2s(&eShapes[2]);
	free_sprite2s(&eShapes[3]);

	/* Normal speed */
	if (fastPlay != 0)
	{
		smoothScroll = true;
		speed = 0x4300;
		JE_resetTimerInt();
		JE_setTimerInt();
	}

	if (play_demo)
	{
		if (demo_file)
		{
			fclose(demo_file);
			demo_file = NULL;
		}

		// Don't stop song if we didn't play one
		if (can_play_audio())
			stop_song();

		fade_black(10);

		//wait_noinput(true, true, true);
	}

	difficultyLevel = oldDifficultyLevel;   /*Return difficulty to normal*/

	if (!play_demo && !record_demo)
	{
		if ((!all_players_dead() || normalBonusLevelCurrent || bonusLevelCurrent) && !playerEndLevel)
		{
			mainLevel = nextLevel;
			JE_endLevelAni();

			fade_song();
		}
		else
		{
			fade_song();
			fade_black(10);
			return;
		}
	}

	if (record_demo)
	{
		DEV_RecordDemoEnd();
		return;
	}

	if (play_demo)
		return;

start_level_first:

	gameNotOverYet = true;

	if (!play_demo)
		attractTic = 0;

	set_volume(tyrMusicVolume, fxVolume);

	endLevel = false;
	reallyEndLevel = false;
	playerEndLevel = false;

	JE_loadMap();

	if (mainLevel == 0)  // if quit itemscreen
		return;          // back to titlescreen

	if (can_play_audio())
		fade_song();

	for (uint i = 0; i < COUNTOF(player); ++i)
		player[i].is_alive = true;

	oldDifficultyLevel = difficultyLevel;
	if (episodeNum == EPISODE_AVAILABLE)
		difficultyLevel--;
	if (difficultyLevel < 1)
		difficultyLevel = 1;

	player[0].x = 100;
	player[0].y = 180;

	player[1].x = 190;
	player[1].y = 180;

	assert(COUNTOF(player->old_x) == COUNTOF(player->old_y));

	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		for (uint j = 0; j < COUNTOF(player->old_x); ++j)
		{
			player[i].old_x[j] = player[i].x - (19 - j);
			player[i].old_y[j] = player[i].y - 18;
		}
		
		player[i].last_x_shot_move = player[i].x;
		player[i].last_y_shot_move = player[i].y;
	}
	
	JE_loadPCX(arcdata_dir(), "newhud.pcx");
	JE_outText(VGAScreen, 268, 189, levelName, 12, 4);

	JE_updateAllOptions();

	JE_showVGA();
	JE_gammaCorrect(&colors, gammaCorrection);
	fade_palette(colors, 50, 0, 255);

	free_sprite2s(&shapes6);
	JE_loadCompShapes(&shapes6, '6'); // explosion sprites

	/* MAPX will already be set correctly */
	mapY = 300 - 8;
	mapY2 = 600 - 8;
	mapY3 = 600 - 8;
	mapYPos = &megaData1.mainmap[mapY][0] - 1;
	mapY2Pos = &megaData2.mainmap[mapY2][0] - 1;
	mapY3Pos = &megaData3.mainmap[mapY3][0] - 1;
	mapXPos = 0;
	mapXOfs = 0;
	mapX2Pos = 0;
	mapX3Pos = 0;
	mapX3Ofs = 0;
	mapXbpPos = 0;
	mapX2bpPos = 0;
	mapX3bpPos = 0;

	map1YDelay = 1;
	map1YDelayMax = 1;
	map2YDelay = 1;
	map2YDelayMax = 1;

	cameraXFocus = -1;

	musicFade = false;

	backPos = 0;
	backPos2 = 0;
	backPos3 = 0;

	starfield_speed = 1;

	/* Setup player ship graphics */
	JE_getShipInfo();

	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		player[i].x_velocity = 0;
		player[i].y_velocity = 0;
		player[i].x_friction_ticks = 0;
		player[i].y_friction_ticks = 0;

		player[i].invulnerable_ticks = 100;

		memset(player[i].shot_repeat, 1, sizeof(player[i].shot_repeat));
		memset(player[i].shot_multi_pos, 0, sizeof(player[i].shot_multi_pos));
		memset(player[i].buttons, 0, sizeof(player[i].buttons));

		// Special data all wiped
		memset(&player[i].specials, 0, sizeof(player[i].specials));
		memset(&player[i].twiddle, 0, sizeof(player[i].twiddle));

		// Don't fire special if holding fire at start
		player[i].shot_repeat[SHOT_SPECIAL] = 2;
		player[i].hud_ready_timer = 0;
		player[i].hud_repeat_start = 1;
	}

	globalFlare = 0;
	globalFlareFilter = -99;

	// Reset level adjusted rank
	ARC_RankLevelAdjusts(0);

	/* Initialize Level Data and Debug Mode */
	levelEnd = 255;
	levelEndWarp = -4;
	deathExplodeSfxWait = 0;
	warningCol = 120;
	warningColChange = 1;
	warningSoundDelay = 0;
	armorShipDelay = 50;

	goingToBonusLevel = false;
	readyToEndLevel = false;
	exitGameTic = 0;

	eventLoc = 1;
	curLoc = 0;
	backMove = 1;
	backMove2 = 2;
	backMove3 = 3;
	explodeMove = 2;
	enemiesActive = true;
	stopBackgrounds = false;
	stopBackgroundNum = 0;
	background3x1   = false;
	background3x1b  = false;
	background3over = 0;
	background2over = 1;
	topEnemyOver = false;
	skyEnemyOverAll = false;
	smallEnemyAdjust = false;
	starActive = true;
	enemyContinualDamage = false;
	levelEnemyFrequency = 96;

	hurryUpTimer = 3000;
	hurryUpLevelLoc = 0;

	for (unsigned int i = 0; i < COUNTOF(boss_bar); i++)
		boss_bar[i].link_num = 0;

	forceEvents = false;  /*Force events to continue if background movement = 0*/

	superEnemy254Jump = 0;   /*When Enemy with PL 254 dies*/

	/* Filter Status */
	filterActive = true;
	filterFade = true;
	filterFadeStart = false;
	levelFilter = -99;
	levelBrightness = -14;
	levelBrightnessChg = 1;

	background2notTransparent = false;

	uint old_weapon_bar[2] = { 0, 0 };  // only redrawn when they change
	uint old_weapon_num[2] = { 0, 0 };

	/* Initial Text */
	JE_drawTextWindow(miscText[20]);

	/* Setup Armor/Shield Data */
	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		player[i].shield_wait = 1;
		player[i].shield      = shield_power[player[i].items.shield];
		player[i].shield_max  = player[i].shield * 2;
	}

	JE_drawShield();
	JE_drawArmor();

	/* Secret Level Display */
	secretLevelDisplayTime = 0;

	if (can_play_audio())
		play_song(levelSong - 1);

	JE_drawPortConfigButtons();

	/* --- MAIN LOOP --- */

	initialize_starfield();

	JE_setNewGameSpeed();

	/* JE_setVol(tyrMusicVolume, fxPlayVol >> 2); NOTE: MXD killed this because it was broken */
	if (!play_demo && record_demo)
		DEV_RecordDemoStart();
	else if (play_demo)
		mt_srand(demo_seed);

	twoPlayerLinked = false;
	twoPlayerLinkReady = false;
	linkGunDirec = M_PI;

	damageRate = 2;  /*Normal Rate for Collision Damage*/

	chargeWait   = 20;
	chargeLevel  = 0;
	chargeGr     = 0;
	chargeGrWait = 3;

	/*Destruction Ratio*/
	totalEnemy = 0;
	enemyKilled[0] = 0;
	enemyKilled[1] = 0;

	astralDuration = 0;

	init_saweapon_bag();

	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		player[i].exploding_ticks = 0;

		player[i].satRotate = 0.0f;
		player[i].attachMove = 0;
		player[i].attachLinked = true;
		player[i].attachReturn = false;
	}

	memset(enemyAvail,       1, sizeof(enemyAvail));
	for (uint i = 0; i < COUNTOF(enemyShotAvail); i++)
		enemyShotAvail[i] = 1;

	/*Initialize Shots*/
	memset(playerShotData,   0, sizeof(playerShotData));
	memset(shotAvail,        0, sizeof(shotAvail));

	memset(globalFlags,      0, sizeof(globalFlags));

	memset(explosions,       0, sizeof(explosions));
	memset(rep_explosions,   0, sizeof(rep_explosions));

	/* --- Clear Sound Queue --- */
	memset(soundQueue,       0, sizeof(soundQueue));
	if (can_play_audio())
		JE_playSampleNumOnChannel(V_GOOD_LUCK, SFXPRIORITY+2);

	memset(enemyShapeTables, 0, sizeof(enemyShapeTables));
	memset(enemy,            0, sizeof(enemy));

	memset(smoothies, 0, sizeof(smoothies));

	levelTimer = false;
	randomExplosions = false;

	last_superpixel = 0;
	memset(superpixels, 0, sizeof(superpixels));

	returnActive = false;

	// keeps map from scrolling past the top
	BKwrap1 = BKwrap1to = &megaData1.mainmap[1][0];
	BKwrap2 = BKwrap2to = &megaData2.mainmap[1][0];
	BKwrap3 = BKwrap3to = &megaData3.mainmap[1][0];

level_loop:

	// Ran out of lives?
	for (uint i = 0; i < 2; ++i)
	{
		if (player[i].player_status == STATUS_INGAME && !player[i].lives)
			ARC_SetPlayerStatus(&player[i], STATUS_NAMEENTRY);
		// Note: if recording a demo, the above skips to STATUS_GAMEOVER
		// If not in the high scores list, the above skips to STATUS_CONTINUE
	}

	// Demo/recording ends when everyone's dead, no continues allowed
	if (play_demo || record_demo)
	{
		// Demo playback ran out of keys
		if (PL_NumPlayers() == 0 || (play_demo && !I_readDemoKeys()))
		{
			reallyEndLevel = true;
			goto start_level;
		}
	}

	// Demo record timer expired
	if (record_demo && SDL_GetTicks() > demo_record_time)
	{
		reallyEndLevel = true;
		goto start_level;
	}

	// Holding up because no players in game
	if (PL_NumPlayers() == 0 && gameNotOverYet)
	{
		old_weapon_bar[0] = 0; // HACK: force bar reset
		old_weapon_bar[1] = 0; // HACK: force bar reset

		memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen->pitch * VGAScreen->h);
		while (PL_NumPlayers() == 0 && gameNotOverYet)
		{
			memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

			wait_delay();
			setjasondelay(frameCountMax);

			I_checkStatus();

			if (player[0].player_status == STATUS_CONTINUE || player[1].player_status == STATUS_CONTINUE)
			{
				JE_word dispTimer = player[0].arc.timer;
				if (player[0].player_status != STATUS_CONTINUE)
					dispTimer = player[1].arc.timer;
				else if (player[1].player_status == STATUS_CONTINUE)
					dispTimer = MAX(player[0].arc.timer, player[1].arc.timer);

				JE_dString(VGAScreenSeg, JE_fontCenter("CONTINUE?", SMALL_FONT_SHAPES), 60, "CONTINUE?", SMALL_FONT_SHAPES);
				sprintf(tmpBuf.s, "%hu", dispTimer);
				JE_dString(VGAScreenSeg, JE_fontCenter(tmpBuf.s, SMALL_FONT_SHAPES), 80, tmpBuf.s, SMALL_FONT_SHAPES);
			}
			JE_showVGA();

			// Failed to continue?
			if (player[0].player_status <= STATUS_NONE && player[1].player_status <= STATUS_NONE)
			{
				gameNotOverYet = false;
				play_song(SONG_GAMEOVER);
				exitGameTic = SDL_GetTicks() + 12000;
			}
		}
	}

	skip_header_undraw = true;

	// Remove view cone if game over
	if (!gameNotOverYet)
		smoothies[6-1] = false;

	I_checkButtons();

	// Input for any player in a status not in game
	if (player[0].player_status != STATUS_INGAME)
		ARC_HandlePlayerStatus(&player[0], 1);
	if (player[1].player_status != STATUS_INGAME)
		ARC_HandlePlayerStatus(&player[1], 2);

	//tempScreenSeg = game_screen; /* side-effect of game_screen */

	starShowVGASpecialCode = smoothies[9-1] + (smoothies[6-1] << 1);

	/*Background Wrapping*/
	if (mapYPos <= BKwrap1)
		mapYPos = BKwrap1to;
	if (mapY2Pos <= BKwrap2)
		mapY2Pos = BKwrap2to;
	if (mapY3Pos <= BKwrap3)
		mapY3Pos = BKwrap3to;

	allPlayersGone = all_players_dead() && (player[0].exploding_ticks == 0 && player[1].exploding_ticks == 0);

	// Music related code
	if (can_play_audio())
	{
		if (musicFade)
		{
			if (tempVolume > 10)
			{
				tempVolume--;
				set_volume(tempVolume, fxVolume);
			}
			else
			{
				musicFade = false;
			}
		}

		if (levelEnd > 0 && endLevel && !all_players_dead())
		{
			play_song(SONG_LEVELEND);
			musicFade = false;
		}
		else if (!playing && gameNotOverYet)
		{
			play_song(levelSong - 1);
		}
	}

	if (!endLevel) // draw HUD
	{
		VGAScreen = VGAScreenSeg; /* side-effect of game_screen */

		// ----- Message bar -----
		if (textErase > 0 && --textErase == 0)
			blit_sprite(VGAScreenSeg, 16, 189, OPTION_SHAPES, 36);  // in-game message area

		// ----- Shield Regen ----
		{
			bool update_shields = false;
			for (uint i = 0; i < 2; ++i)
			{
				if (!PL_Alive(i) || player[i].shield >= player[i].shield_max)
					continue;

				// If a shield draining twiddle is active, don't replenish shields
				else if (player[i].specials.flare_control != 0 && special[player[i].specials.flare_special].pwr < 100)
					player[i].shield_wait = 16;

				// If wait time exhausted, replenish
				else if (!(--player[i].shield_wait))
				{
					player[i].shield_wait = 16;
					++player[i].shield;
					update_shields = true;
				}
			}

			if (update_shields)
				JE_drawShield();
		}

		// ----- Weapon power ----
		for (uint i = 0; i < 2; ++i)
		{
			if (player[i].player_status != STATUS_INGAME)
			{
				old_weapon_bar[i] = 0; // HACK: force bar reset
				continue;
			}

			uint item_power = player[i].items.power_level;

			if (old_weapon_bar[i] != player[i].items.power_level || old_weapon_num[i] != player[i].port_mode)
			{
				int x = (i == 0) ? 4 : 308;
				int y;
				JE_byte cc, c, bri;

				old_weapon_bar[i] = item_power;
				old_weapon_num[i] = player[i].port_mode;

				fill_rectangle_xy(VGAScreenSeg, x, 8, x + 7, 62, 0);

				JE_colorFromPWeapon(player[i].port_mode, &c, &bri);
				cc = (c * 16) + bri + 2;
				y = 59;

				for (uint j = 0; j < item_power; ++j)
				{
					fill_rectangle_xy(VGAScreenSeg, x, y, x+7, y+3, cc);
					fill_rectangle_xy(VGAScreenSeg, x, y, x, y+3, cc+1);
					JE_pix(VGAScreenSeg, x, y, cc+3);
					fill_rectangle_xy(VGAScreenSeg, x+7, y, x+7, y+3, cc-2);
					y -= 5;
				}

				x = (i == 0) ? 20 : 300;
				sprintf(tmpBuf.s, "%d", item_power);
				fill_rectangle_xy(VGAScreenSeg, x-5, 55, x + 4, 62, 0);
				JE_outText(VGAScreenSeg, x - JE_textWidth(tmpBuf.s, TINY_FONT) / 2, 56, tmpBuf.s, 12, 1);
			}
		}

		oldMapX3Ofs = mapX3Ofs;

		enemyOnScreen = 0;
	}

	/* use game_screen for all the generic drawing functions */
	VGAScreen = game_screen;

	/*---------------------------EVENTS-------------------------*/
	while (eventRec[eventLoc-1].eventtime <= curLoc && eventLoc <= maxEvent)
		JE_eventSystem();

	/* SMOOTHIES! */
	JE_checkSmoothies();
	if (anySmoothies)
		VGAScreen = VGAScreen2;  // this makes things complicated, but we do it anyway :(

	/* --- BACKGROUNDS --- */
	/* --- BACKGROUND 1 --- */

	if (forceEvents && !backMove)
		curLoc++;

	if (map1YDelayMax > 1 && backMove < 2)
		backMove = (map1YDelay == 1) ? 1 : 0;

	/*Draw background*/
	if (astralDuration == 0)
		draw_background_1(VGAScreen);
	else
		JE_clr256(VGAScreen);

	/*Set Movement of background 1*/
	if (--map1YDelay == 0)
	{
		map1YDelay = map1YDelayMax;

		curLoc += backMove;

		backPos += backMove;

		if (backPos > 27)
		{
			backPos -= 28;
			mapY--;
			mapYPos -= 14;  /*Map Width*/
		}
	}

	if (starActive || astralDuration > 0)
	{
		update_and_draw_starfield(VGAScreen, starfield_speed);
	}

	if (processorType > 1 && smoothies[5-1])
	{
		iced_blur_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}

	/*-----------------------BACKGROUNDS------------------------*/
	/*-----------------------BACKGROUND 2------------------------*/
	if (background2over == 3)
	{
		draw_background_2(VGAScreen);
		background2 = true;
	}

	if (background2over == 0)
	{
		if (!(smoothies[2-1] && processorType < 4) && !(smoothies[1-1] && processorType == 3))
		{
			if (wild && !background2notTransparent)
				draw_background_2_blend(VGAScreen);
			else
				draw_background_2(VGAScreen);
		}
	}

	if (smoothies[0] && processorType > 2 && smoothie_data[0] == 0)
	{
		lava_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}
	if (smoothies[2-1] && processorType > 2)
	{
		water_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}

	/*-----------------------Ground Enemy------------------------*/
	lastEnemyOnScreen = enemyOnScreen;

	tempMapXOfs = mapXOfs;
	tempBackMove = backMove;
	JE_drawEnemy(50);
	JE_drawEnemy(100);

	if (enemyOnScreen == 0 || enemyOnScreen == lastEnemyOnScreen)
	{
		if (stopBackgroundNum == 1)
			stopBackgroundNum = 9;
	}

	if (smoothies[0] && processorType > 2 && smoothie_data[0] > 0)
	{
		lava_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}

	if (superWild)
	{
		neat += 3;
		JE_darkenBackground(neat);
	}

	/*-----------------------BACKGROUNDS------------------------*/
	/*-----------------------BACKGROUND 2------------------------*/
	if (!(smoothies[2-1] && processorType < 4) &&
	    !(smoothies[1-1] && processorType == 3))
	{
		if (background2over == 1)
		{
			if (wild && !background2notTransparent)
				draw_background_2_blend(VGAScreen);
			else
				draw_background_2(VGAScreen);
		}
	}

	if (superWild)
	{
		neat++;
		JE_darkenBackground(neat);
	}

	if (background3over == 2)
		draw_background_3(VGAScreen);

	/* New Enemy */
	if (enemiesActive && mt_rand() % 100 > levelEnemyFrequency)
	{
		tempW = levelEnemy[mt_rand() % levelEnemyMax];
		if (tempW == 2)
			soundQueue[3] = S_WEAPON_7;
		b = JE_newEnemy(0, tempW, 0);
	}

	if (processorType > 1 && smoothies[3-1])
	{
		iced_blur_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}
	if (processorType > 1 && smoothies[4-1])
	{
		blur_filter(game_screen, VGAScreen);
		VGAScreen = game_screen;
	}

	/* Draw Sky Enemy */
	if (!skyEnemyOverAll)
	{
		lastEnemyOnScreen = enemyOnScreen;

		tempMapXOfs = mapX2Ofs;
		tempBackMove = 0;
		JE_drawEnemy(25);

		if (enemyOnScreen == lastEnemyOnScreen)
		{
			if (stopBackgroundNum == 2)
				stopBackgroundNum = 9;
		}
	}

	if (background3over == 0)
		draw_background_3(VGAScreen);

	/* Draw Top Enemy */
	if (!topEnemyOver)
	{
		tempMapXOfs = (background3x1 == 0) ? oldMapX3Ofs : mapXOfs;
		tempBackMove = backMove3;
		JE_drawEnemy(75);
	}

	/* Player Shot Images */
	JE_byte zinglon_player = 0xFF;
	for (int z = 0; z < MAX_PWEAPON + 2; z++)
	{
		bool is_special = false;
		int tempShotX = 0, tempShotY = 0;
		JE_word tempX2, tempY2;
		JE_byte filter, doIced, playerNum;
		JE_integer damage;
		JE_word shrapnel;
		JE_boolean infiniteShot;

		if (z >= MAX_PWEAPON)
		{
			zinglon_player = (z - MAX_PWEAPON);
			if (!player[zinglon_player].specials.zinglon)
				continue;

			filter = 0;
			playerNum = zinglon_player;
			damage = 1000; // note: soul of zinglon does not deal damage -- it kills enemies that are weak
			doIced = 0;
			infiniteShot = false;
			shrapnel = 0;
		}
		else
		{
			if (shotAvail[z] == 0 || !player_shot_move_and_draw(z, &is_special, &tempShotX, &tempShotY, &tempX2, &tempY2))
				continue;

			filter = playerShotData[z].shotBlastFilter;
			playerNum = playerShotData[z].playerNumber;
			damage = playerShotData[z].shotDmg;
			shrapnel = playerShotData[z].shrapnel;
			infiniteShot = playerShotData[z].infinite;
			doIced = playerShotData[z].ice * 10;			
		}

		for (b = 0; b < 100; b++)
		{
			if (enemyAvail[b] == 0)
			{
				bool collided;

				if (zinglon_player != 0xFF)
				{
					temp = 25 - abs(player[zinglon_player].specials.zinglon - 25);
					collided = abs(enemy[b].ex + enemy[b].mapoffset - (player[zinglon_player].x + 7)) < temp;
				}
				else if (is_special)
				{
					collided = ((enemy[b].enemycycle == 0) &&
					            (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX - tempX2) < (25 + tempX2)) &&
					            (abs(enemy[b].ey - tempShotY - 12 - tempY2)                 < (29 + tempY2))) ||
					           ((enemy[b].enemycycle > 0) &&
					            (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX - tempX2) < (13 + tempX2)) &&
					            (abs(enemy[b].ey - tempShotY - 6 - tempY2)                  < (15 + tempY2)));
				}
				else
				{
					collided = ((enemy[b].enemycycle == 0) &&
					            (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX) < 25) && (abs(enemy[b].ey - tempShotY - 12) < 29)) ||
					           ((enemy[b].enemycycle > 0) &&
					            (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX) < 13) && (abs(enemy[b].ey - tempShotY - 6) < 15));
				}

				if (collided)
				{
					int orig_health = enemy[b].ehealth;

					temp = enemy[b].linknum;
					if (temp == 0)
						temp = 255;

					if (damage > 0 && enemy[b].ehealth != ENEMY_INVULNERABLE)
					{
						for (unsigned int i = 0; i < COUNTOF(boss_bar); i++)
							if (temp == boss_bar[i].link_num)
								boss_bar[i].color = 6;

						if (enemy[b].enemyground)
							enemy[b].filter = filter;

						for (unsigned int e = 0; e < COUNTOF(enemy); e++)
						{
							if (enemy[e].linknum == temp &&
							    enemyAvail[e] != 1 &&
							    enemy[e].enemyground != 0)
							{
								if (doIced)
									enemy[e].iced = doIced;
								enemy[e].filter = filter;
							}
						}
					}

					if (orig_health > damage)
					{
						if (zinglon_player == 0xFF)
						{
							if (enemy[b].ehealth != ENEMY_INVULNERABLE)
							{
								enemy[b].ehealth -= damage;
								JE_setupExplosion(tempShotX, tempShotY, 0, 0, false, false);
							}
							else
							{
								JE_doSP(tempShotX + 6, tempShotY + 6, damage / 200 + 3, damage / 400 + 2, filter);
							}
						}

						soundQueue[5] = S_ENEMY_HIT;

						if ((orig_health - damage <= (int)enemy[b].edlevel * 100) &&
						    ((!enemy[b].edamaged) ^ (enemy[b].edani < 0)))
						{

							for (temp3 = 0; temp3 < 100; temp3++)
							{
								if (enemyAvail[temp3] != 1)
								{
									int linknum = enemy[temp3].linknum;
									if (
									     (temp3 == b) ||
									     (
									       (temp != 255) &&
									       (
									         ((enemy[temp3].edlevel > 0) && (linknum == temp)) ||
									         (
									           (enemyContinualDamage && (temp - 100 == linknum)) ||
									           ((linknum > 40) && (linknum / 20 == temp / 20) && (linknum <= temp))
									         )
									       )
									     )
									   )
									{
										enemy[temp3].enemycycle = 1;

										enemy[temp3].edamaged = !enemy[temp3].edamaged;

										if (enemy[temp3].edani != 0)
										{
											enemy[temp3].ani = abs(enemy[temp3].edani);
											enemy[temp3].aniactive = 1;
											enemy[temp3].animax = 0;
											enemy[temp3].animin = enemy[temp3].edgr;
											enemy[temp3].enemycycle = enemy[temp3].animin - 1;

										}
										else if (enemy[temp3].edgr > 0)
										{
											enemy[temp3].egr[1-1] = enemy[temp3].edgr;
											enemy[temp3].ani = 1;
											enemy[temp3].aniactive = 0;
											enemy[temp3].animax = 0;
											enemy[temp3].animin = 1;
										}
										else
										{
											enemyAvail[temp3] = 1;
											++enemyKilled[playerNum - 1];
										}

										enemy[temp3].aniwhenfire = 0;

										if (enemy[temp3].ehealth > (Uint32)enemy[temp3].edlevel * 100)
											enemy[temp3].ehealth = (Uint32)enemy[temp3].edlevel * 100;

										tempX = enemy[temp3].ex + enemy[temp3].mapoffset;
										tempY = enemy[temp3].ey;

										if (enemyDat[enemy[temp3].enemytype].esize != 1)
											JE_setupExplosion(tempX, tempY - 6, 0, 1, false, false);
										else
											JE_setupExplosionLarge(enemy[temp3].enemyground, enemy[temp3].explonum / 2, tempX, tempY);
									}
								}
							}
						}
					}
					else
					{

						if ((temp == 254) && (superEnemy254Jump > 0))
							JE_eventJump(superEnemy254Jump);

						for (temp2 = 0; temp2 < 100; temp2++)
						{
							if (enemyAvail[temp2] != 1)
							{
								temp3 = enemy[temp2].linknum;
								if ((temp2 == b) || (temp == 254) ||
								    ((temp != 255) && ((temp == temp3) || (temp - 100 == temp3)
								    || ((temp3 > 40) && (temp3 / 20 == temp / 20) && (temp3 <= temp)))))
								{

									int enemy_screen_x = enemy[temp2].ex + enemy[temp2].mapoffset;

									if (enemy[temp2].special)
									{
										assert((unsigned int) enemy[temp2].flagnum-1 < COUNTOF(globalFlags));
										globalFlags[enemy[temp2].flagnum-1] = enemy[temp2].setto;
									}

									if (enemy[temp2].enemydie > 0 && enemyDat[enemy[temp2].enemydie].value != 30000)
									{
										int temp_b = b;
										// ---

										tempW = enemy[temp2].enemydie;
										const int enemy_offset = (enemyDat[tempW].value > 30000) ? 100 : (temp2 - (temp2 % 25));
										b = JE_newEnemy(enemy_offset, tempW, 0);

										if (b != 0)
										{
											if (enemy[b-1].evalue > 30000)
											{
												JE_byte temp = SAPowerupBag[superArcadePowerUp++];
												if (superArcadePowerUp == 5)
													randomize_saweapon_bag();
												enemy[b-1].egr[1-1] = 5 + temp * 2;
												enemy[b-1].evalue = 30000 + temp;
												enemy[b-1].scoreitem = true;
											}

											// Bugfix (original game): Don't set scoreitem here
											// JE_makeEnemy already does a good enough job of that
											// All we do by setting it here is ruin it and mess up stuff 
											//if (enemy[b-1].evalue != 0)
											//	enemy[b-1].scoreitem = true;
											//else
											//	enemy[b-1].scoreitem = false;

											enemy[b-1].ex = enemy[temp2].ex;
											enemy[b-1].ey = enemy[temp2].ey;
										}

										// ---
										b = temp_b;
									}

									if ((enemy[temp2].evalue > 0) && (enemy[temp2].evalue < 10000))
									{
										player[playerNum - 1].cash += enemy[temp2].evalue;
									}

									if ((enemy[temp2].edlevel == -1) && (temp == temp3))
									{
										enemy[temp2].edlevel = 0;
										enemyAvail[temp2] = 2;
										enemy[temp2].egr[1-1] = enemy[temp2].edgr;
										enemy[temp2].ani = 1;
										enemy[temp2].aniactive = 0;
										enemy[temp2].animax = 0;
										enemy[temp2].animin = 1;
										enemy[temp2].edamaged = true;
										enemy[temp2].enemycycle = 1;
									} else {
										enemyAvail[temp2] = 1;
										++enemyKilled[playerNum - 1];
									}

									if (enemyDat[enemy[temp2].enemytype].esize == 1)
									{
										JE_setupExplosionLarge(enemy[temp2].enemyground, enemy[temp2].explonum, enemy_screen_x, enemy[temp2].ey);
										soundQueue[6] = S_EXPLOSION_9;
									}
									else
									{
										JE_setupExplosion(enemy_screen_x, enemy[temp2].ey, 0, 1, false, false);
										soundQueue[6] = S_EXPLOSION_8;
									}
								}
							}
						}
					}

					if (infiniteShot && shrapnel > 0)
						b = player_shot_create(0, SHOT_MISC, tempShotX, tempShotY, shrapnel, playerNum);

					if (!infiniteShot && zinglon_player == 0xFF)
					{
						if (damage <= orig_health)
						{
							if (shrapnel > 0)
								b = player_shot_create(0, SHOT_MISC, tempShotX, tempShotY, shrapnel, playerNum);

							shotAvail[z] = 0;
							goto draw_player_shot_loop_end;
						}
						else
						{
							playerShotData[z].shotDmg -= orig_health;
							damage = playerShotData[z].shotDmg;
						}
					}
				}
			}
		}



draw_player_shot_loop_end:
			;
	}

	/* Player movement indicators for shots that track your ship */
	for (uint i = 0; i < COUNTOF(player); ++i)
	{
		player[i].last_x_shot_move = player[i].x;
		player[i].last_y_shot_move = player[i].y;
	}
	
	/*=================================*/
	/*=======Collisions Detection======*/
	/*=================================*/
	
	for (uint i = 0; i < 2; ++i)
		if (PL_Alive(i) && !endLevel)
			JE_playerCollide(&player[i], i + 1);

	if (gameNotOverYet)
		JE_mainGamePlayerFunctions();      /*--------PLAYER DRAW+MOVEMENT---------*/

	if (record_demo)
		DEV_RecordDemoInput();

	if (!endLevel)
	{    /*MAIN DRAWING IS STOPPED STARTING HERE*/

		/* Draw Enemy Shots */
		for (int z = 0; z < ENEMY_SHOT_MAX; z++)
		{
			if (enemyShotAvail[z] == 0)
			{
				enemyShot[z].sxm += enemyShot[z].sxc;
				enemyShot[z].sx += enemyShot[z].sxm;

				if (enemyShot[z].tx != 0)
				{
					if (enemyShot[z].sx > player[0].x)
					{
						if (enemyShot[z].sxm > -enemyShot[z].tx)
						{
							enemyShot[z].sxm--;
						}
					} else {
						if (enemyShot[z].sxm < enemyShot[z].tx)
						{
							enemyShot[z].sxm++;
						}
					}
				}

				enemyShot[z].sym += enemyShot[z].syc;
				enemyShot[z].sy += enemyShot[z].sym;

				if (enemyShot[z].ty != 0)
				{
					if (enemyShot[z].sy > player[0].y)
					{
						if (enemyShot[z].sym > -enemyShot[z].ty)
						{
							enemyShot[z].sym--;
						}
					} else {
						if (enemyShot[z].sym < enemyShot[z].ty)
						{
							enemyShot[z].sym++;
						}
					}
				}

				if (enemyShot[z].duration-- == 0 || enemyShot[z].sy > 190 || enemyShot[z].sy <= -14 || enemyShot[z].sx > 275 || enemyShot[z].sx <= 0)
				{
					enemyShotAvail[z] = true;
				}
				else  // check if shot collided with player
				{
					for (uint i = 0; i < 2; ++i)
					{
						if (PL_Alive(i) && !(player[i].is_dragonwing && twoPlayerLinked) &&
						    enemyShot[z].sx > player[i].x - (signed)player[i].shot_hit_area_x &&
						    enemyShot[z].sx < player[i].x + (signed)player[i].shot_hit_area_x &&
						    enemyShot[z].sy > player[i].y - (signed)player[i].shot_hit_area_y &&
						    enemyShot[z].sy < player[i].y + (signed)player[i].shot_hit_area_y)
						{
							tempX = enemyShot[z].sx;
							tempY = enemyShot[z].sy;
							temp = enemyShot[z].sdmg;

							enemyShotAvail[z] = true;

							JE_setupExplosion(tempX, tempY, 0, 0, false, false);

							if (player[i].invulnerable_ticks == 0)
							{
								if ((temp = PL_PlayerDamage(&player[i], temp)) > 0)
								{
									player[i].x_velocity += (enemyShot[z].sxm * temp) / 2;
									player[i].y_velocity += (enemyShot[z].sym * temp) / 2;
								}
							}

							break;
						}
					}

					if (enemyShotAvail[z] == false)
					{
						if (enemyShot[z].animax != 0)
						{
							if (++enemyShot[z].animate >= enemyShot[z].animax)
								enemyShot[z].animate = 0;
						}

						if (enemyShot[z].sgr >= 500)
							blit_sprite2(VGAScreen, enemyShot[z].sx, enemyShot[z].sy, shotShapes[1], enemyShot[z].sgr + enemyShot[z].animate - 500);
						else
							blit_sprite2(VGAScreen, enemyShot[z].sx, enemyShot[z].sy, shotShapes[0], enemyShot[z].sgr + enemyShot[z].animate);
					}
				}

			}
		}
	}

	if (background3over == 1)
		draw_background_3(VGAScreen);

	/* Draw Top Enemy */
	if (topEnemyOver)
	{
		tempMapXOfs = (background3x1 == 0) ? oldMapX3Ofs : oldMapXOfs;
		tempBackMove = backMove3;
		JE_drawEnemy(75);
	}

	/* Draw Sky Enemy */
	if (skyEnemyOverAll)
	{
		lastEnemyOnScreen = enemyOnScreen;

		tempMapXOfs = mapX2Ofs;
		tempBackMove = 0;
		JE_drawEnemy(25);

		if (enemyOnScreen == lastEnemyOnScreen)
		{
			if (stopBackgroundNum == 2)
				stopBackgroundNum = 9;
		}
	}

	// Priority enemies
	tempMapXOfs = mapX2Ofs;
	tempBackMove = 0;
	JE_drawEnemy(125);

	/*-------------------------- Sequenced Explosions -------------------------*/
	enemyStillExploding = false;
	for (int i = 0; i < MAX_REPEATING_EXPLOSIONS; i++)
	{
		if (rep_explosions[i].ttl != 0)
		{
			if (rep_explosions[i].delay > 65536)
			{
				printf("warning: something stuffed garbage data (%d, %d) into rep_explosions[%d] again\n", rep_explosions[i].ttl, rep_explosions[i].delay, i);
				rep_explosions[i].delay = rep_explosions[i].ttl = 0;
				continue;
			}
			enemyStillExploding = true;

			if (rep_explosions[i].delay > 0)
			{
				rep_explosions[i].delay--;
				continue;
			}

			rep_explosions[i].y += backMove2 + 1;
			tempX = rep_explosions[i].x + (mt_rand() % 24) - 12;
			tempY = rep_explosions[i].y + (mt_rand() % 27) - 24;

			if (rep_explosions[i].big)
			{
				JE_setupExplosionLarge(false, 2, tempX, tempY);

				if (rep_explosions[i].ttl == 1 || mt_rand() % 5 == 1)
					soundQueue[7] = S_EXPLOSION_11;
				else
					soundQueue[6] = S_EXPLOSION_9;

				rep_explosions[i].delay = 4 + (mt_rand() % 3);
			}
			else
			{
				JE_setupExplosion(tempX, tempY, 0, 1, false, false);

				soundQueue[5] = S_EXPLOSION_4;

				rep_explosions[i].delay = 3;
			}

			rep_explosions[i].ttl--;
		}
	}

	/*---------------------------- Draw Explosions ----------------------------*/
	for (int j = 0; j < MAX_EXPLOSIONS; j++)
	{
		if (explosions[j].ttl != 0)
		{
			if (explosions[j].fixed_position != true)
			{
				explosions[j].sprite++;
				explosions[j].y += explodeMove;
			}
			else if (explosions[j].follow_player == true)
			{
				explosions[j].x += explosionFollowAmountX;
				explosions[j].y += explosionFollowAmountY;
			}
			explosions[j].y += explosions[j].delta_y;
			explosions[j].x += explosions[j].delta_x;

			if (explosions[j].y > 200 - 14)
			{
				explosions[j].ttl = 0;
			}
			else
			{
				if (explosionTransparent)
					blit_sprite2_blend(VGAScreen, explosions[j].x, explosions[j].y, shapes6, explosions[j].sprite + 1);
				else
					blit_sprite2(VGAScreen, explosions[j].x, explosions[j].y, shapes6, explosions[j].sprite + 1);

				explosions[j].ttl--;
			}
		}
	}

	/*-----------------------BACKGROUNDS------------------------*/
	/*-----------------------BACKGROUND 2------------------------*/
	if (!(smoothies[2-1] && processorType < 4) &&
	    !(smoothies[1-1] && processorType == 3))
	{
		if (background2over == 2)
		{
			if (wild && !background2notTransparent)
				draw_background_2_blend(VGAScreen);
			else
				draw_background_2(VGAScreen);
		}
	}

	/*-------------------------Warning---------------------------*/
	if ((PL_Alive(0) && player[0].armor < 6) ||
	    (PL_Alive(1) && player[1].armor < 6))
	{
		int armor_amount;
		if (!PL_Alive(0))
			armor_amount = player[1].armor;
		else if (!PL_Alive(1))
			armor_amount = player[0].armor;
		else
			armor_amount = MIN(player[0].armor, player[1].armor);

		if (armorShipDelay > 0)
		{
			armorShipDelay--;
		}
		else
		{
			tempW = 560;
			b = JE_newEnemy(50, tempW, 0);
			if (b > 0)
			{
				enemy[b-1].enemydie = 990 + (mt_rand() % 3) + 1;
				enemy[b-1].eyc -= backMove3;
				enemy[b-1].ehealth = 360;
			}
			armorShipDelay = 500;
		}

		tempW = armor_amount * 4 + 8;
		if (warningSoundDelay > tempW)
			warningSoundDelay = tempW;

		if (warningSoundDelay > 1)
		{
			warningSoundDelay--;
		}
		else
		{
			soundQueue[7] = S_WARNING;
			warningSoundDelay = tempW;
		}

		warningCol += warningColChange;
		if (warningCol > 113 + (14 - (armor_amount * 2)))
		{
			warningColChange = -warningColChange;
			warningCol = 113 + (14 - (armor_amount * 2));
		}
		else if (warningCol < 113)
		{
			warningColChange = -warningColChange;
		}
		fill_rectangle_xy(VGAScreen, 24, 181, 138, 183, warningCol);
		fill_rectangle_xy(VGAScreen, 175, 181, 287, 183, warningCol);
		fill_rectangle_xy(VGAScreen, 24, 0, 287, 3, warningCol);

		JE_outText(VGAScreen, 140, 178, "WARNING", 7, (warningCol % 16) / 2);

		JE_drawArmor();

	}
	else
	{
		// Make sure the armor ship has a fixed delay to first show up
		armorShipDelay = 300;
	}

	/*------- Random Explosions --------*/
	if (randomExplosions && mt_rand() % 10 == 1)
		JE_setupExplosionLarge(false, 20, mt_rand() % 280, mt_rand() % 180);

	/*=================================*/
	/*=======The Sound Routine=========*/
	/*=================================*/
	if (can_play_audio())
	{
		temp = 0;
		for (temp2 = 0; temp2 < STANDARD_SFX_CHANNELS; temp2++)
		{
			if (soundQueue[temp2] != S_NONE)
			{
				temp = soundQueue[temp2];
				if (temp2 == 3)
					temp3 = fxPlayVol;
				else if (temp == 15)
					temp3 = fxPlayVol / 4;
				else   /*Lightning*/
					temp3 = fxPlayVol / 2;

				JE_multiSamplePlay(digiFx[temp-1], fxSize[temp-1], temp2, temp3);

				soundQueue[temp2] = S_NONE;
			}
		}
	}

	if (returnActive && enemyOnScreen == 0)
	{
		JE_eventJump(65535);
		returnActive = false;
	}

	/*-------      DEbug      ---------*/
	debugTime = SDL_GetTicks();
	//tempW = lastmouse_but;
	//tempX = mouse_x;
	//tempY = mouse_y;
	tempW = tempX = tempY = -1;

	if (debug)
	{
		strcpy(tempStr, "");
		for (temp = 0; temp < 9; temp++)
		{
			sprintf(tempStr, "%s%c", tempStr,  smoothies[temp] + 48);
		}
		sprintf(buffer, "SM = %s", tempStr);
		JE_outText(VGAScreen, 30, 70, buffer, 4, 0);

		sprintf(buffer, "Memory left = %d", -1);
		JE_outText(VGAScreen, 30, 80, buffer, 4, 0);
		sprintf(buffer, "Enemies onscreen = %d", enemyOnScreen);
		JE_outText(VGAScreen, 30, 90, buffer, 6, 0);

		debugHist = debugHist + abs((JE_longint)debugTime - (JE_longint)lastDebugTime);
		debugHistCount++;
		sprintf(tempStr, "%2.3f", 1000.0f / roundf(debugHist / debugHistCount));
		sprintf(buffer, "X:%d Y:%-5d  %s FPS  %d %d %d %d", (mapX - 1) * 12 + player[0].x, curLoc, tempStr, player[0].x_velocity, player[0].y_velocity, player[0].x, player[0].y);
		JE_outText(VGAScreen, 45, 175, buffer, 15, 3);
		lastDebugTime = debugTime;
	}

	if (secretLevelDisplayTime > 0)
	{
		const JE_byte flash[8] = {-6, -5, -4, -3, -2, -3, -4, -5};
		JE_outTextAndDarken(VGAScreen, 90, 14, miscText[59], 15, flash[(--secretLevelDisplayTime) & 7], FONT_SHAPES);
	}

	/*Pentium Speed Mode?*/
	if (pentiumMode)
	{
		frameCountMax = (frameCountMax == 2) ? 3 : 2;
	}

	/*--------  Level Timer    ---------*/
	ARC_Timers();

	/*GAME OVER*/
	if ((normalBonusLevelCurrent || bonusLevelCurrent) && allPlayersGone)
		reallyEndLevel = true;

	// possibly redundant
	if (!gameNotOverYet && play_demo)
		reallyEndLevel = true;

	// time out, or button to return to title
	if (!gameNotOverYet)
	{
		if (I_inputMade(INPUT_P1_FIRE) || I_inputMade(INPUT_P2_FIRE))
			reallyEndLevel = true;
		else if (SDL_GetTicks() > exitGameTic)
			reallyEndLevel = true;
	}

	if (play_demo) // input kills demo
	{
		if (I_inputMade(INPUT_P1_FIRE) || I_inputMade(INPUT_P2_FIRE))
			reallyEndLevel = true;
	}
	else // Check for hot debug menu
	{
		if (I_inputMade(INPUT_SERVICE_HOTDEBUG))
			Menu_hotDebugScreen();
	}

	/** Test **/
	JE_drawSP();

	/*Filtration*/
	if (filterActive)
	{
		JE_filterScreen(levelFilter, levelBrightness);
	}

	draw_boss_bar();

	VGAScreen = VGAScreenSeg; /* side-effect of game_screen */

	JE_copyGameToVGA();

	if (!gameNotOverYet)
		JE_dString(VGAScreenSeg, JE_fontCenter(miscText[21], FONT_SHAPES), 60, miscText[21], FONT_SHAPES); // game over

	JE_inGameDisplays();

	if (debug)
	{
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "evnLoc: %d", eventLoc);
		JE_textShade(VGAScreen, 32, 100, tmpBuf.s, 7, 2, FULL_SHADE);
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "curLoc: %d", curLoc);
		JE_textShade(VGAScreen, 32, 108, tmpBuf.s, 7, 2, FULL_SHADE);
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "huryUp: %d", hurryUpTimer);
		JE_textShade(VGAScreen, 32, 116, tmpBuf.s, 7, 2, FULL_SHADE);

		memset(tmpBuf.s, 0, sizeof(tmpBuf.s));
		for (temp = 0; temp < COUNTOF(globalFlags); ++temp)
			tmpBuf.s[temp] = (globalFlags[temp]) ? 'T' : 'F';
		snprintf(tmpBuf.l, sizeof(tmpBuf.l), "flags: %s", tmpBuf.s);
		JE_textShade(VGAScreen, 32, 124, tmpBuf.l, 7, 2, FULL_SHADE);
	}

	// make sure we have a clean picture for the end level animation, if we need it
	if (reallyEndLevel)
		skip_header_undraw = false;

	JE_showVGA();


	/*Start backgrounds if no enemies on screen
	  End level if number of enemies left to kill equals 0.*/
	if (stopBackgroundNum == 9 && backMove == 0 && !enemyStillExploding)
	{
		backMove = 1;
		backMove2 = 2;
		backMove3 = 3;
		explodeMove = 2;
		stopBackgroundNum = 0;
		stopBackgrounds = false;
		if (waitToEndLevel)
		{
			endLevel = true;
			levelEnd = 40;
		}
		reallyEndLevel = allPlayersGone;
	}

	if (!endLevel && enemyOnScreen == 0)
	{
		// STOP CORE'S COUNTDOWN WHEN IT'S DEAD OMG SDFKJNFSJLN
		if (readyToEndLevel && enemyStillExploding)
		{
			if (levelTimerCountdown > 0)
				levelTimer = false;
		}
		else if (readyToEndLevel)
		{
			if (levelTimerCountdown > 0)
				levelTimer = false;
			readyToEndLevel = false;
			endLevel = true;
			levelEnd = 40;
			reallyEndLevel = allPlayersGone;
		}
		if (stopBackgrounds)
		{
			stopBackgrounds = false;
			backMove = 1;
			backMove2 = 2;
			backMove3 = 3;
			explodeMove = 2;
		}
	}

	if (reallyEndLevel)
	{
		goto start_level;
	}
	goto level_loop;
}

static bool read_episode_sections( void )
{
	// Load LEVELS.DAT - Section = MAINLEVEL
	FILE *ep_f = dir_fopen_die(data_dir(), episode_file, "rb");

	char s[256];
	Uint8 pic_buffer[320*200]; /* screen buffer, 8-bit specific */
	Uint8 *vga, *pic, *vga2; /* screen pointer, 8-bit specific */

	while (mainLevel != 0)
	{
		// Seek Section # Mainlevel
		fseek(ep_f, 0, SEEK_SET);
		int onSection = 0;
		while (onSection < mainLevel)
		{
			read_encrypted_pascal_string(s, sizeof(s), ep_f);
			if (s[0] == '*')
			{
				onSection++;
				s[0] = ' ';
			}
			if (feof(ep_f))
			{
				fprintf(stderr, "error: requested section number %hu exceeds maximum of %d\n", mainLevel, onSection);
				JE_tyrianHalt(1);
			}
		}
		printf("on section number %d\n", onSection);

		do
		{
			strcpy(s, " ");
			read_encrypted_pascal_string(s, sizeof(s), ep_f);
			printf("%s\n", s);

			if (s[0] != ']')
				continue;

			switch (s[1])
			{
			case 'L':
				nextLevel = atoi(s + 9);
				if (nextLevel == 0)
					nextLevel = mainLevel + 1;

				SDL_strlcpy(levelName, s + 13, 10);
				levelSong = atoi(s + 22);
				lvlFileNum = atoi(s + 25);
				bonusLevelCurrent = (strlen(s) > 28) & (s[28] == '$');
				normalBonusLevelCurrent = (strlen(s) > 27) & (s[27] == '$');

				fclose(ep_f);
				return true; // level loaded

			case 'A':
				JE_playAnim("tyrend.anm", 0, 7);
				break;

			case 'G':
				mapOrigin = atoi(s + 4);
				mapPNum   = atoi(s + 7);
				for (int i = 0; i < mapPNum; i++)
				{
					mapPlanet[i] = atoi(s + 1 + (i + 1) * 8);
					mapSection[i] = atoi(s + 4 + (i + 1) * 8);
				}
				break;

			case '?': // Data cubes -- ignore
			case '!': // Data cubes -- ignore
			case '+': // Data cubes -- ignore
				break;

			case 'g': // Galaga
			case 'x': // Extra game
			case 'e': // ENGAGE
				printf("hit a sidegame mode flag -- ignoring\n");
				break;

			case 'J':  // section jump
				mainLevel = atoi(s + 3);
				break;

			case '2':  // two-player section jump
				printf("arcade section jump hit (always true)\n");
				mainLevel = atoi(s + 3);
				break;

			case 'w':  // Stalker 21.126 section jump
				printf("21.126 jump hit (always false)\n");
				break;

			case 't':
				if (levelTimer && levelTimerCountdown == 0)
				{
					printf("timer check jump hit, jumping (failed last stage)\n");
					mainLevel = atoi(s + 3);
				}
				else
					printf("timer check jump hit, not jumping (completed last stage)\n");
				break;

			case 'l':
				// note: this check was inverted, now succeeds if anyone is alive
				if (all_players_dead())
					mainLevel = atoi(s + 3);
				break;

			case 's': // store savepoint -- ignore
			case 'b': // save game before start -- ignore
			case 'i': // set buy song -- ignore
				break;

			case 'I': // Load Items Available Information
				// We skip the item screen and just jump immediately
				mainLevel = mapSection[mapPNum-1];
				break;

			case '@':
				useLastBank = !useLastBank;
				break;

			case 'Q': // Episode end
				play_song(SONG_EPISODEEND);

				fade_black(15);
				JE_clr256(VGAScreen);
				memcpy(colors, palettes[6-1], sizeof(colors));
				fade_palette(colors, 15, 0, 255);
				JE_loadPCX(arcdata_dir(), "select.pcx");

				temp = secretHint + (mt_rand() % 3) * 3;

				JE_byte plrs = PL_WhosInGame();
				levelWarningLines = 0;
				if (plrs & 1)
				{
					snprintf(levelWarningText[levelWarningLines++], sizeof(*levelWarningText),
						"%s %lu", miscText[40], player[0].cash);
				}
				if (plrs & 2)
				{
					snprintf(levelWarningText[levelWarningLines++], sizeof(*levelWarningText),
						"%s %lu", miscText[40], player[1].cash);
				}
				strcpy(levelWarningText[levelWarningLines++], "");

				for (int i = 0; i < temp - 1; i++)
				{
					do
						read_encrypted_pascal_string(s, sizeof(s), ep_f);
					while (s[0] != '#');
				}

				do
				{
					read_encrypted_pascal_string(s, sizeof(s), ep_f);
					strcpy(levelWarningText[levelWarningLines], s);
					levelWarningLines++;
				}
				while (s[0] != '#');
				levelWarningLines--;

				frameCountMax = 4;
				JE_displayText();

				fade_black(15);

				// If out of episodes, play the credits and leave.
				if (!Episode_next())
				{
					mainLevel = 0;
					skip_header_draw = false;
					JE_playCredits();
				}
				else
				{
					play_song(SONG_NEXTEPISODE);

					JE_clr256(VGAScreen);
					memcpy(colors, palettes[6-1], sizeof(colors));

					JE_dString(VGAScreen, JE_fontCenter(episode_name[episodeNum], SMALL_FONT_SHAPES), 130, episode_name[episodeNum], SMALL_FONT_SHAPES);
					JE_dString(VGAScreen, JE_fontCenter(miscText[5-1], SMALL_FONT_SHAPES), 185, miscText[5-1], SMALL_FONT_SHAPES);

					JE_showVGA();
					fade_palette(colors, 15, 0, 255);

					do
					{
						SDL_Delay(16);
					} while (!I_checkSkipScene());

					fade_black(15);

					mainLevel = FIRST_LEVEL;
					fclose(ep_f); // Time to leave this episode behind us
					return read_episode_sections(); // And go to the new one
				}
				break;

			case 'P':
				printf("pic P\n");
				tempX = atoi(s + 3);
				if (tempX > 900)
				{
					memcpy(colors, palettes[pic_pal[tempX-1 - 900]], sizeof(colors));
					JE_clr256(VGAScreen);
					JE_showVGA();
					fade_palette(colors, 1, 0, 255);
				}
				else
				{
					if (tempX == 0)
						JE_loadPCX(data_dir(), "tshp2.pcx");
					else
						JE_loadPic(VGAScreen, tempX, false);

					JE_showVGA();
					fade_palette(colors, 10, 0, 255);
				}
				break;

			case 'U':
				printf("pic U\n");
				memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

				tempX = atoi(s + 3);
				JE_loadPic(VGAScreen, tempX, false);
				memcpy(pic_buffer, VGAScreen->pixels, sizeof(pic_buffer));

				I_checkButtons();

				for (int z = 0; z <= 199; z++)
				{
					if (true /* !newkey */)
					{
						vga = VGAScreen->pixels;
						vga2 = VGAScreen2->pixels;
						pic = pic_buffer + (199 - z) * 320;

						setjasondelay(1); /* attempting to emulate JE_waitRetrace();*/

						for (y = 0; y <= 199; y++)
						{
							if (y <= z)
							{
								memcpy(vga, pic, 320);
								pic += 320;
							}
							else
							{
								memcpy(vga, vga2, VGAScreen->pitch);
								vga2 += VGAScreen->pitch;
							}
							vga += VGAScreen->pitch;
						}

						JE_showVGA();
						service_wait_delay();
					}
				}

				memcpy(VGAScreen->pixels, pic_buffer, sizeof(pic_buffer));
				break;

			case 'V':
				printf("pic V\n");
				memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

				tempX = atoi(s + 3);
				JE_loadPic(VGAScreen, tempX, false);
				memcpy(pic_buffer, VGAScreen->pixels, sizeof(pic_buffer));

				I_checkButtons();
				for (int z = 0; z <= 199; z++)
				{
					if (true /* !newkey */)
					{
						vga = VGAScreen->pixels;
						vga2 = VGAScreen2->pixels;
						pic = pic_buffer;

						setjasondelay(1); /* attempting to emulate JE_waitRetrace();*/

						for (y = 0; y < 199; y++)
						{
							if (y <= 199 - z)
							{
								memcpy(vga, vga2, VGAScreen->pitch);
								vga2 += VGAScreen->pitch;
							}
							else
							{
								memcpy(vga, pic, 320);
								pic += 320;
							}
							vga += VGAScreen->pitch;
						}

						JE_showVGA();
						service_wait_delay();
					}
				}

				memcpy(VGAScreen->pixels, pic_buffer, sizeof(pic_buffer));
				break;

			case 'R':
				printf("pic R\n");
				memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

				tempX = atoi(s + 3);
				JE_loadPic(VGAScreen, tempX, false);
				memcpy(pic_buffer, VGAScreen->pixels, sizeof(pic_buffer));

				I_checkButtons();

				for (int z = 0; z <= 318; z++)
				{
					if (true /* !newkey */)
					{
						vga = VGAScreen->pixels;
						vga2 = VGAScreen2->pixels;
						pic = pic_buffer;

						setjasondelay(1); /* attempting to emulate JE_waitRetrace();*/

						for(y = 0; y < 200; y++)
						{
							memcpy(vga, vga2 + z, 319 - z);
							vga += 320 - z;
							vga2 += VGAScreen2->pitch;
							memcpy(vga, pic, z + 1);
							vga += z;
							pic += 320;
						}

						JE_showVGA();
						service_wait_delay();
					}
				}

				memcpy(VGAScreen->pixels, pic_buffer, sizeof(pic_buffer));
				break;

			case 'C':
				fade_black(10);
				JE_clr256(VGAScreen);
				JE_showVGA();
				memcpy(colors, palettes[7], sizeof(colors));
				set_palette(colors, 0, 255);
				break;

			case 'B':
				fade_black(10);
				break;
			case 'F':
				fade_white(100);
				fade_black(30);
				JE_clr256(VGAScreen);
				JE_showVGA();
				break;

			case 'W':
				if (true /* !ESCPressed */)
				{
					warningCol = 14 * 16 + 5;
					warningColChange = 1;
					warningSoundDelay = 0;
					levelWarningDisplay = (s[2] == 'y');
					levelWarningLines = 0;
					frameCountMax = atoi(s + 4);
					setjasondelay2(6);
					warningRed = frameCountMax / 10;
					frameCountMax = frameCountMax % 10;

					do
					{
						read_encrypted_pascal_string(s, sizeof(s), ep_f);

						if (s[0] != '#')
							strncpy(levelWarningText[levelWarningLines++], s, 60);
					}
					while (!(s[0] == '#'));

					JE_displayText();
				}
				break;

			case 'H':
				if (initialDifficulty < 3)
				{
					mainLevel = atoi(s + 4);
				}
				break;

			case 'h':
				if (initialDifficulty > 2)
				{
					read_encrypted_pascal_string(s, sizeof(s), ep_f);
				}
				break;

			case 'S': // net only -- ignore
			case 'n':
				break;

			case 'M':
				temp = atoi(s + 3);
				play_song(temp - 1);
				break;
			}
		} while (onSection == mainLevel);
	}

	fclose(ep_f); // if quitting in any way, 
	return false; // go back to title screen
}

/* --- Load Level/Map Data --- */
void JE_loadMap( void )
{
	JE_DanCShape shape;

	JE_word x, y;
	JE_integer yy;
	JE_word mapSh[3][128]; /* [1..3, 0..127] */
	JE_byte *ref[3][128]; /* [1..3, 0..127] */

	JE_byte mapBuf[15 * 600]; /* [1..15 * 600] */
	JE_word bufLoc;

	useLastBank = false;

	if (!play_demo)
	{
		// Hide the header until we're in game
		skip_header_draw = true;
		if (!read_episode_sections())
			return;
		fade_black(50);
		skip_header_draw = false;
	}

	arcTextTimer = 0;

	FILE *level_f = dir_fopen_die(data_dir(), level_file, "rb");

	printf("num: %d, location: $%x\n", lvlFileNum, lvlPos[(lvlFileNum-1) * 2]);
	MOD_PatcherInit(lvlFileNum);

	fseek(level_f, lvlPos[(lvlFileNum-1) * 2], SEEK_SET);

	fgetc(level_f); // char_mapFile
	JE_char char_shapeFile = fgetc(level_f);
	efread(&mapX,  sizeof(JE_word), 1, level_f);
	efread(&mapX2, sizeof(JE_word), 1, level_f);
	efread(&mapX3, sizeof(JE_word), 1, level_f);

	efread(&levelEnemyMax, sizeof(JE_word), 1, level_f);
	for (x = 0; x < levelEnemyMax; x++)
	{
		efread(&levelEnemy[x], sizeof(JE_word), 1, level_f);
	}

	efread(&maxEvent, sizeof(JE_word), 1, level_f);
	for (x = 0; x < maxEvent; x++)
	{
		if (MOD_Patcher(eventRec, &x))
		{
			// skip event (overridden)
			fseek(level_f, 11, SEEK_CUR);
			continue;
		}
		efread(&eventRec[x].eventtime, sizeof(JE_word), 1, level_f);
		efread(&eventRec[x].eventtype, sizeof(JE_byte), 1, level_f);
		efread(&eventRec[x].eventdat,  sizeof(JE_integer), 1, level_f);
		efread(&eventRec[x].eventdat2, sizeof(JE_integer), 1, level_f);
		efread(&eventRec[x].eventdat3, sizeof(JE_shortint), 1, level_f);
		efread(&eventRec[x].eventdat5, sizeof(JE_shortint), 1, level_f);
		efread(&eventRec[x].eventdat6, sizeof(JE_shortint), 1, level_f);
		efread(&eventRec[x].eventdat4, sizeof(JE_byte), 1, level_f);
	}
	eventRec[x].eventtime = 65500;  /*Not needed but just in case*/
	MOD_PatcherClose();

	/*debuginfo('Level loaded.');*/

	/*debuginfo('Loading Map');*/

	/* MAP SHAPE LOOKUP TABLE - Each map is directly after level */
	efread(mapSh, sizeof(JE_word), sizeof(mapSh) / sizeof(JE_word), level_f);
	for (temp = 0; temp < 3; temp++)
	{
		for (temp2 = 0; temp2 < 128; temp2++)
		{
			mapSh[temp][temp2] = SDL_Swap16(mapSh[temp][temp2]);
		}
	}

	/* Read Shapes.DAT */
	sprintf(tempStr, "shapes%c.dat", tolower((unsigned char)char_shapeFile));
	FILE *shpFile = dir_fopen_die(data_dir(), tempStr, "rb");

	for (int z = 0; z < 600; z++)
	{
		JE_boolean shapeBlank = fgetc(shpFile);

		if (shapeBlank)
			memset(shape, 0, sizeof(shape));
		else
			efread(shape, sizeof(JE_byte), sizeof(shape), shpFile);

		/* Match 1 */
		for (int x = 0; x <= 71; ++x)
		{
			if (mapSh[0][x] == z+1)
			{
				memcpy(megaData1.shapes[x].sh, shape, sizeof(JE_DanCShape));

				ref[0][x] = (JE_byte *)megaData1.shapes[x].sh;
			}
		}

		/* Match 2 */
		for (int x = 0; x <= 71; ++x)
		{
			if (mapSh[1][x] == z+1)
			{
				if (x != 71 && !shapeBlank)
				{
					memcpy(megaData2.shapes[x].sh, shape, sizeof(JE_DanCShape));

					y = 1;
					for (yy = 0; yy < (24 * 28) >> 1; yy++)
						if (shape[yy] == 0)
							y = 0;

					megaData2.shapes[x].fill = y;
					ref[1][x] = (JE_byte *)megaData2.shapes[x].sh;
				}
				else
				{
					ref[1][x] = NULL;
				}
			}
		}

		/*Match 3*/
		for (int x = 0; x <= 71; ++x)
		{
			if (mapSh[2][x] == z+1)
			{
				if (x < 70 && !shapeBlank)
				{
					memcpy(megaData3.shapes[x].sh, shape, sizeof(JE_DanCShape));

					y = 1;
					for (yy = 0; yy < (24 * 28) >> 1; yy++)
						if (shape[yy] == 0)
							y = 0;

					megaData3.shapes[x].fill = y;
					ref[2][x] = (JE_byte *)megaData3.shapes[x].sh;
				}
				else
				{
					ref[2][x] = NULL;
				}
			}
		}
	}

	fclose(shpFile);

	efread(mapBuf, sizeof(JE_byte), 14 * 300, level_f);
	bufLoc = 0;              /* MAP NUMBER 1 */
	for (y = 0; y < 300; y++)
	{
		for (x = 0; x < 14; x++)
		{
			megaData1.mainmap[y][x] = ref[0][mapBuf[bufLoc]];
			bufLoc++;
		}
	}

	efread(mapBuf, sizeof(JE_byte), 14 * 600, level_f);
	bufLoc = 0;              /* MAP NUMBER 2 */
	for (y = 0; y < 600; y++)
	{
		for (x = 0; x < 14; x++)
		{
			megaData2.mainmap[y][x] = ref[1][mapBuf[bufLoc]];
			bufLoc++;
		}
	}

	efread(mapBuf, sizeof(JE_byte), 15 * 600, level_f);
	bufLoc = 0;              /* MAP NUMBER 3 */
	for (y = 0; y < 600; y++)
	{
		for (x = 0; x < 15; x++)
		{
			megaData3.mainmap[y][x] = ref[2][mapBuf[bufLoc]];
			bufLoc++;
		}
	}

	fclose(level_f);

	/* Note: The map data is automatically calculated with the correct mapsh
	value and then the pointer is calculated using the formula (MAPSH-1)*168.
	Then, we'll automatically add S2Ofs to get the exact offset location into
	the shape table! This makes it VERY FAST! */

	/*debuginfo('Map file done.');*/
	/* End of find loop for LEVEL??.DAT */
}

void intro_logos( void )
{
	SDL_FillRect(VGAScreen, NULL, 0);

	fade_white(50);

	JE_loadPic(VGAScreen, 10, false);
	JE_showVGA();

	fade_palette(colors, 50, 0, 255);

	setjasondelay(200);
	wait_delayorinput();

	fade_black(10);

	JE_loadPic(VGAScreen, 12, false);
	JE_showVGA();

	fade_palette(colors, 10, 0, 255);

	setjasondelay(200);
	wait_delayorinput();

	fade_black(10);
}

void JE_displayText( void )
{
	hasRequestedToSkip = false;

	/* Display Warning Text */
	tempY = 55;
	if (warningRed)
	{
		tempY = 2;
	}
	for (temp = 0; temp < levelWarningLines; temp++)
	{
		if (true /* !ESCPressed */)
		{
			JE_outCharGlow(10, tempY, levelWarningText[temp]);

			tempY += 10;
		}
	}

	if (!hasRequestedToSkip)
	{
		frameCountMax = 6;
		temp = 1;
	} else {
		temp = 0;
	}

	textGlowFont = TINY_FONT;
	tempW = 184;
	if (warningRed)
	{
		tempW = 7 * 16 + 6;
	}

	JE_outCharGlow(JE_fontCenter("Press Fire...", TINY_FONT), tempW, "Press Fire...");

	do
	{
		if (levelWarningDisplay)
		{
			JE_updateWarning(VGAScreen);
		}

		setjasondelay(1);

		wait_delay();

	} while (!I_checkSkipScene() && !(hasRequestedToSkip && temp == 1));
	levelWarningDisplay = false;
}

Sint16 JE_newEnemy( int enemyOffset, Uint16 eDatI, JE_byte shapeTableOverride )
{
	int i = enemyOffset, stop = enemyOffset + 25;
	for (; i < stop; ++i)
	{
		if (enemyAvail[i] == 1)
		{
			enemyAvail[i] = JE_makeEnemy(&enemy[i], eDatI, shapeTableOverride);
			return i + 1;
		}
	}
	printf("JE_newEnemy: no empty slots for enemy %hu\n", eDatI);
	return 0;
}

uint JE_makeEnemy( struct JE_SingleEnemyType *enemy, Uint16 eDatI, JE_byte shapeTableOverride )
{
	uint avail, i;
	JE_byte shapeTableI;

	switch (enemyDat[eDatI].value)
	{
		case -5: eDatI = 901; break; // Switch old Hot Dog powerup drop
		case -4: eDatI = 900; break; // Switch old SuperBomb powerup drop
		case -2: eDatI = 533; break; // Make all shield powerups look the same
		default: break;
	}

	switch ((shapeTableI = shapeTableOverride > 0 ? shapeTableOverride : enemyDat[eDatI].shapebank))
	{
		case 21: // Coins and Gems -- always loaded into pickupShapes
			enemy->sprite2s = &pickupShapes;
			break;
		case 26: // Icons -- always loaded into iconShapes
			enemy->sprite2s = &iconShapes;
			break;
		default:
			for (i = 0; i < COUNTOF(enemyShapeTables); ++i)
			{
				if (enemyShapeTables[i] == shapeTableI)
				{
					enemy->sprite2s = &eShapes[i];
					goto shape_found; // double-break
				}
			}
			// maintain buggy Tyrian behavior (use shape table value from previous enemy that occupied this index in the enemy array)
			fprintf(stderr, "warning: ignoring sprite from unloaded shape table %d\n", shapeTableI);
			break;
	}

	shape_found:
	enemy->enemydatofs = &enemyDat[eDatI];

	enemy->mapoffset = 0;

	for (i = 0; i < 3; ++i)
	{
		enemy->eshotmultipos[i] = 0;
	}

	enemy->enemyground = (enemyDat[eDatI].explosiontype & 1) == 0;
	enemy->explonum = enemyDat[eDatI].explosiontype >> 1;

	enemy->launchfreq = enemyDat[eDatI].elaunchfreq;
	enemy->launchwait = enemyDat[eDatI].elaunchfreq;

	// T2000 ... Account for the second enemy bank only if we're creating something from it
	if (eDatI > 1000)
	{
		enemy->launchtype = enemyDat[eDatI].elaunchtype;
		enemy->launchspecial = 0;
	}
	else
	{
		enemy->launchtype = enemyDat[eDatI].elaunchtype % 1000;
		enemy->launchspecial = enemyDat[eDatI].elaunchtype / 1000;
	}

	enemy->xaccel = enemyDat[eDatI].xaccel;
	enemy->yaccel = enemyDat[eDatI].yaccel;

	enemy->xminbounce = -10000;
	enemy->xmaxbounce = 10000;
	enemy->yminbounce = -10000;
	enemy->ymaxbounce = 10000;
	/*Far enough away to be impossible to reach*/

	for (i = 0; i < 3; ++i)
	{
		enemy->tur[i] = enemyDat[eDatI].tur[i];
	}

	enemy->ani = enemyDat[eDatI].ani;
	enemy->animin = 1;

	switch (enemyDat[eDatI].animate)
	{
	case 0:
		enemy->enemycycle = 1;
		enemy->aniactive = 0;
		enemy->animax = 0;
		enemy->aniwhenfire = 0;
		break;
	case 1:
		enemy->enemycycle = 0;
		enemy->aniactive = 1;
		enemy->animax = 0;
		enemy->aniwhenfire = 0;
		break;
	case 2:
		enemy->enemycycle = 1;
		enemy->aniactive = 2;
		enemy->animax = enemy->ani;
		enemy->aniwhenfire = 2;
		break;
	}

	if (enemyDat[eDatI].startxc != 0)
		enemy->ex = enemyDat[eDatI].startx + (mt_rand() % (enemyDat[eDatI].startxc * 2)) - enemyDat[eDatI].startxc + 1;
	else
		enemy->ex = enemyDat[eDatI].startx + 1;

	if (enemyDat[eDatI].startyc != 0)
		enemy->ey = enemyDat[eDatI].starty + (mt_rand() % (enemyDat[eDatI].startyc * 2)) - enemyDat[eDatI].startyc + 1;
	else
		enemy->ey = enemyDat[eDatI].starty + 1;

	enemy->exc = enemyDat[eDatI].xmove;
	enemy->eyc = enemyDat[eDatI].ymove;
	enemy->excc = enemyDat[eDatI].xcaccel;
	enemy->eycc = enemyDat[eDatI].ycaccel;
	enemy->exccw = abs(enemy->excc);
	enemy->exccwmax = enemy->exccw;
	enemy->eyccw = abs(enemy->eycc);
	enemy->eyccwmax = enemy->eyccw;
	enemy->exccadd = (enemy->excc > 0) ? 1 : -1;
	enemy->eyccadd = (enemy->eycc > 0) ? 1 : -1;
	enemy->special = false;
	enemy->iced = 0;

	if (enemyDat[eDatI].xrev == 0)
		enemy->exrev = 100;
	else if (enemyDat[eDatI].xrev == -99)
		enemy->exrev = 0;
	else
		enemy->exrev = enemyDat[eDatI].xrev;

	if (enemyDat[eDatI].yrev == 0)
		enemy->eyrev = 100;
	else if (enemyDat[eDatI].yrev == -99)
		enemy->eyrev = 0;
	else
		enemy->eyrev = enemyDat[eDatI].yrev;

	enemy->exca = (enemy->xaccel > 0) ? 1 : -1;
	enemy->eyca = (enemy->yaccel > 0) ? 1 : -1;

	enemy->enemytype = eDatI;

	for (i = 0; i < 3; ++i)
	{
		if (enemy->tur[i] == 252)
			enemy->eshotwait[i] = 1;
		else if (enemy->tur[i] > 0)
			enemy->eshotwait[i] = 20;
		else
			enemy->eshotwait[i] = 255;
	}
	for (i = 0; i < 20; ++i)
		enemy->egr[i] = enemyDat[eDatI].egraphic[i];
	enemy->size = enemyDat[eDatI].esize;
	enemy->linknum = 0;
	enemy->edamaged = enemyDat[eDatI].dani < 0;
	enemy->enemydie = enemyDat[eDatI].eenemydie;

	enemy->freq[1-1] = enemyDat[eDatI].freq[1-1];
	enemy->freq[2-1] = enemyDat[eDatI].freq[2-1];
	enemy->freq[3-1] = enemyDat[eDatI].freq[3-1];

	enemy->edani   = enemyDat[eDatI].dani;
	enemy->edgr    = enemyDat[eDatI].dgr;
	enemy->edlevel = enemyDat[eDatI].dlevel;

	enemy->fixedmovey = 0;

	enemy->filter = 0x00;

	enemy->evalue = enemyDat[eDatI].value;
	if (DIP.rankAffectsScore && enemy->evalue > 1 && enemy->evalue < 10000)
	{
		switch (difficultyLevel)
		{
			default: break;
			case 3:  enemy->evalue = enemyDat[eDatI].value * 1.125f; break;
			case 4:  enemy->evalue = enemyDat[eDatI].value * 1.5f;   break;
			case 5:  enemy->evalue = enemyDat[eDatI].value * 2;      break;
			case 6:  enemy->evalue = enemyDat[eDatI].value * 2.5f;   break;
			case 7:
			case 8:  enemy->evalue = enemyDat[eDatI].value * 4;      break;
			case 9:
			case 10: enemy->evalue = enemyDat[eDatI].value * 8;      break;
		}
		if (enemy->evalue > 9000)
			enemy->evalue = 9000;
	}

	if (enemyDat[eDatI].armor > 0)
	{
		if (enemyDat[eDatI].armor != 255)
		{
			int tempArmor = enemyDat[eDatI].armor * 100;
			switch (difficultyLevel)
			{
				default: break;
				case 4:  tempArmor *= 1.1f;   break;
				case 5:  tempArmor *= 1.2f;   break;
				case 6:  tempArmor *= 1.3f;   break;
				case 7:  tempArmor *= 1.5f;   break;
				case 8:  tempArmor *= 1.7f;   break;
				case 9:  tempArmor *= 1.8f;   break;
				case 10: tempArmor *= 2;      break;
			}

			// Episode 4 was designed for players already loaded up in full game mode,
			// so tone down the armor values a little bit
			if (episodeNum == 4)
				tempArmor *= 0.9f;

			if (tempArmor >= ENEMY_INVULNERABLE)
				tempArmor = ENEMY_INVULNERABLE - 1;
			enemy->ehealth = tempArmor;
		}
		else
		{
			enemy->ehealth = ENEMY_INVULNERABLE;
		}

		avail = 0;
		enemy->scoreitem = false;
	}
	else
	{
		avail = 2;
		enemy->ehealth = ENEMY_INVULNERABLE;
		// Bugfix (original game): Make sure scoreitem is always set
		enemy->scoreitem = (enemy->evalue != 0);
	}

	if (!enemy->scoreitem)
	{
		totalEnemy++;  /*Destruction ratio*/
	}

	enemy->playertotarget = PL_RandomPlayer();

	/* indicates what to set ENEMYAVAIL to */
	return avail;
}

void JE_createNewEventEnemy( JE_byte enemyTypeOfs, JE_word enemyOffset, JE_byte shapeTableOverride )
{
	int i;

	b = 0;

	for(i = enemyOffset; i < enemyOffset + 25; i++)
	{
		if (enemyAvail[i] == 1)
		{
			b = i + 1;
			break;
		}
	}

	if (b == 0)
	{
		return;
	}

	tempW = eventRec[eventLoc-1].eventdat + enemyTypeOfs;

	enemyAvail[b-1] = JE_makeEnemy(&enemy[b-1], tempW, shapeTableOverride);

	// T2000
	// When T2000 gives an X position of -200, what it actually wants is a random X position...
	if (eventRec[eventLoc-1].eventdat2 == -200)
	{
		// This implementation is black-boxed

		// Ranged 24 - 231
		eventRec[eventLoc-1].eventdat2 = (mt_rand() % 208) + 24;
	}

	if (eventRec[eventLoc-1].eventdat2 != -99)
	{
		switch (enemyOffset)
		{
		case 0:
			enemy[b-1].ex = eventRec[eventLoc-1].eventdat2 - (mapX - 1) * 24;
			enemy[b-1].ey -= backMove2;
			break;
		case 25:
		case 75:
			enemy[b-1].ex = eventRec[eventLoc-1].eventdat2 - (mapX - 1) * 24 - 12;
			enemy[b-1].ey -= backMove;
			break;
		case 50:
			if (background3x1)
			{
				enemy[b-1].ex = eventRec[eventLoc-1].eventdat2 - (mapX - 1) * 24 - 12;
			} else {
				enemy[b-1].ex = eventRec[eventLoc-1].eventdat2 - mapX3 * 24 - 24 * 2 + 6;
			}
			enemy[b-1].ey -= backMove3;

			if (background3x1b)
			{
				enemy[b-1].ex -= 6;
			}
			break;
		}
		enemy[b-1].ey = -28;
		if (background3x1b && enemyOffset == 50)
		{
			enemy[b-1].ey += 4;
		}
	}

	if (smallEnemyAdjust && enemy[b-1].size == 0)
	{
		enemy[b-1].ex -= 10;
		enemy[b-1].ey -= 7;
	}

	enemy[b-1].ey += eventRec[eventLoc-1].eventdat5;
	enemy[b-1].eyc += eventRec[eventLoc-1].eventdat3;
	enemy[b-1].linknum = eventRec[eventLoc-1].eventdat4;
	enemy[b-1].fixedmovey = eventRec[eventLoc-1].eventdat6;
}

void JE_eventJump( JE_word jump )
{
	JE_word tempW;

	if (jump == 65535)
	{
		curLoc = returnLoc;
	}
	else
	{
		returnLoc = curLoc + 1;
		curLoc = jump;
	}
	tempW = 0;
	do
	{
		tempW++;
	}
	while (!(eventRec[tempW-1].eventtime >= curLoc));
	eventLoc = tempW - 1;
}

bool JE_searchFor/*enemy*/( JE_byte PLType, JE_byte* out_index )
{
	int found_id = -1;

	for (int i = 0; i < 100; i++)
	{
		if (enemyAvail[i] == 0 && enemy[i].linknum == PLType)
		{
			found_id = i;
		}
	}

	if (found_id != -1) {
		if (out_index) {
			*out_index = found_id;
		}
		return true;
	} else {
		return false;
	}
}

void JE_eventSystem( void )
{
#ifdef EVENT_LOGGING
	const JE_word prEventLoc = eventLoc;
	printf("time %d, ev %d: %d ", curLoc, eventLoc, eventRec[eventLoc-1].eventtype);
#define event_name(s) printf(s)
#else
#define event_name(s)
#endif

	switch (eventRec[eventLoc-1].eventtype)
	{
	case 0:
		event_name("no-op");
		break;

	case 1:
		event_name("starfield_speed");
		starfield_speed = eventRec[eventLoc-1].eventdat;
		break;

	case 2:
		event_name("map_back_move");
		map1YDelay = 1;
		map1YDelayMax = 1;
		map2YDelay = 1;
		map2YDelayMax = 1;

		backMove = eventRec[eventLoc-1].eventdat;
		backMove2 = eventRec[eventLoc-1].eventdat2;

		if (backMove2 > 0)
			explodeMove = backMove2;
		else
			explodeMove = backMove;

		backMove3 = eventRec[eventLoc-1].eventdat3;

		if (backMove > 0)
			stopBackgroundNum = 0;
		break;

	case 3:
		event_name("map_reset_move?");
		backMove = 1;
		map1YDelay = 3;
		map1YDelayMax = 3;
		backMove2 = 1;
		map2YDelay = 2;
		map2YDelayMax = 2;
		backMove3 = 1;
		break;

	case 4:
		event_name("map_stop");
		stopBackgrounds = true;
		switch (eventRec[eventLoc-1].eventdat)
		{
		case 0:
		case 1:
			stopBackgroundNum = 1;
			break;
		case 2:
			stopBackgroundNum = 2;
			break;
		case 3:
			stopBackgroundNum = 3;
			break;
		}
		break;

	case 5:  // load enemy shape banks
		event_name("load_enemy_shape_banks");
		{
			const JE_char shapeFile[] =
			{
				'2', '4', '7', '8', 'A', 'B', 'C', 'D', 'E', 'F',
				'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',	'O', 'P',
				'Q', 'R', 'S', 'T', 'U', '5', '#', 'V', '0', '@',
				'3', '^', '5', '9', /* T2000   */ '\'', '%'
			};

			Uint8 newEnemyShapeTables[] =
			{
				eventRec[eventLoc-1].eventdat > 0 ? eventRec[eventLoc-1].eventdat : 0,
				eventRec[eventLoc-1].eventdat2 > 0 ? eventRec[eventLoc-1].eventdat2 : 0,
				eventRec[eventLoc-1].eventdat3 > 0 ? eventRec[eventLoc-1].eventdat3 : 0,
				eventRec[eventLoc-1].eventdat4 > 0 ? eventRec[eventLoc-1].eventdat4 : 0,
			};
			
			for (unsigned int i = 0; i < COUNTOF(newEnemyShapeTables); ++i)
			{
				if (enemyShapeTables[i] != newEnemyShapeTables[i])
				{
					if (newEnemyShapeTables[i] > 0)
					{
						assert(newEnemyShapeTables[i] <= COUNTOF(shapeFile));
						JE_loadCompShapes(&eShapes[i], shapeFile[newEnemyShapeTables[i] - 1]);
					}
					else
						free_sprite2s(&eShapes[i]);

					enemyShapeTables[i] = newEnemyShapeTables[i];
				}
			}
		}
		break;

	case 6: /* Ground Enemy */
		event_name("new_ground_enemy");
		JE_createNewEventEnemy(0, 25, 0);
		break;

	case 7: /* Top Enemy */
		event_name("new_top_enemy");
		JE_createNewEventEnemy(0, 50, 0);
		break;

	case 8:
		event_name("star_inactive");
		starActive = false;
		break;

	case 9:
		event_name("star_active");
		starActive = true;
		break;

	case 10: /* Ground Enemy 2 */
		event_name("new_ground2_enemy");
		JE_createNewEventEnemy(0, 75, 0);
		break;

	case 11:
		event_name("end_level");
		if (allPlayersGone || eventRec[eventLoc-1].eventdat == 1)
			reallyEndLevel = true;
		else
			if (!endLevel)
			{
				readyToEndLevel = false;
				endLevel = true;
				levelEnd = 40;
			}
		break;

	case 12: /* Custom 4x4 Ground Enemy */
		event_name("new_ground4x4custom_enemy");
		{
			uint temp = 0;
			switch (eventRec[eventLoc-1].eventdat6)
			{
			case 0:
			case 1:
				temp = 25;
				break;
			case 2:
				temp = 0;
				break;
			case 3:
				temp = 50;
				break;
			case 4:
				temp = 75;
				break;
			}
			eventRec[eventLoc-1].eventdat6 = 0;   /* We use EVENTDAT6 for the background */

			// T2000
			// When T2000 gives an X position of -200, what it actually wants is a random X position...
			if (eventRec[eventLoc-1].eventdat2 == -200)
			{
				// This implementation is black-boxed

				// Ranged 24 - 231
				eventRec[eventLoc-1].eventdat2 = (mt_rand() % 208) + 24;
			}

			JE_createNewEventEnemy(0, temp, 0);

			JE_createNewEventEnemy(1, temp, 0);
			if (b == 0)
				break;
			enemy[b-1].ex += 24;

			JE_createNewEventEnemy(2, temp, 0);
			if (b == 0)
				break;
			enemy[b-1].ey -= 28;

			JE_createNewEventEnemy(3, temp, 0);
			if (b == 0)
				break;
			enemy[b-1].ex += 24;
			enemy[b-1].ey -= 28;

			break;
		}
	case 13:
		event_name("enemies_inactive");
		enemiesActive = false;
		break;

	case 14:
		event_name("enemies_active");
		enemiesActive = true;
		break;

	case 15: /* Sky Enemy */
		event_name("new_sky_enemy");
		JE_createNewEventEnemy(0, 0, 0);
		break;

	case 16:
		event_name("susan_callout");
		{
			const JE_byte windowTextSamples[9] =
			{
				V_DANGER,
				V_BOSS,
				V_ENEMIES,
				V_CLEARED_PLATFORM,
				V_DANGER,
				V_SPIKES,
				V_ACCELERATE,
				V_DANGER,
				V_ENEMIES
			};
			if (eventRec[eventLoc-1].eventdat > 9)
			{
				fprintf(stderr, "warning: event 16: bad event data\n");
			}
			else
			{
				JE_drawTextWindow(outputs[eventRec[eventLoc-1].eventdat-1]);

				// Prioirity given to Susan's voice lines
				if (can_play_audio())
					JE_playSampleNumOnChannel(windowTextSamples[eventRec[eventLoc-1].eventdat-1], SFXPRIORITY+2);
			}
		}
		break;

	case 17: /* Ground Bottom */
		event_name("new_groundbottom_enemy");
		JE_createNewEventEnemy(0, 25, 0);
		if (b > 0)
		{
			enemy[b-1].ey = 190 + eventRec[eventLoc-1].eventdat5;
		}
		break;

	case 18: /* Sky Enemy on Bottom */
		event_name("new_skybottom_enemy");
		JE_createNewEventEnemy(0, 0, 0);
		if (b > 0)
		{
			enemy[b-1].ey = 190 + eventRec[eventLoc-1].eventdat5;
		}
		break;

	case 19: /* Enemy Global Move */
		event_name("enemy_global_move");
	{
		int initial_i = 0, max_i = 0;
		bool all_enemies = false;

		if (eventRec[eventLoc-1].eventdat3 > 79 && eventRec[eventLoc-1].eventdat3 < 90)
		{
			initial_i = 0;
			max_i = 100;
			all_enemies = false;
			eventRec[eventLoc-1].eventdat4 = newPL[eventRec[eventLoc-1].eventdat3 - 80];
		}
		else
		{
			switch (eventRec[eventLoc-1].eventdat3)
			{
			case 0:
				initial_i = 0;
				max_i = 100;
				all_enemies = false;
				break;
			case 2:
				initial_i = 0;
				max_i = 25;
				all_enemies = true;
				break;
			case 1:
				initial_i = 25;
				max_i = 50;
				all_enemies = true;
				break;
			case 3:
				initial_i = 50;
				max_i = 75;
				all_enemies = true;
				break;
			case 99:
				initial_i = 0;
				max_i = 100;
				all_enemies = true;
				break;
			}
		}

		for (int i = initial_i; i < max_i; i++)
		{
			if (all_enemies || enemy[i].linknum == eventRec[eventLoc-1].eventdat4)
			{
				if (eventRec[eventLoc-1].eventdat != -99)
					enemy[i].exc = eventRec[eventLoc-1].eventdat;

				if (eventRec[eventLoc-1].eventdat2 != -99)
					enemy[i].eyc = eventRec[eventLoc-1].eventdat2;

				if (eventRec[eventLoc-1].eventdat6 != 0)
					enemy[i].fixedmovey = eventRec[eventLoc-1].eventdat6;

				if (eventRec[eventLoc-1].eventdat6 == -99)
					enemy[i].fixedmovey = 0;

				if (eventRec[eventLoc-1].eventdat5 > 0)
					enemy[i].enemycycle = eventRec[eventLoc-1].eventdat5;
			}
		}
		break;
	}

	case 20: /* Enemy Global Accel */
		event_name("enemy_global_accel");
		if (eventRec[eventLoc-1].eventdat3 > 79 && eventRec[eventLoc-1].eventdat3 < 90)
			eventRec[eventLoc-1].eventdat4 = newPL[eventRec[eventLoc-1].eventdat3 - 80];

		for (temp = 0; temp < 100; temp++)
		{
			if (enemyAvail[temp] != 1
			    && (enemy[temp].linknum == eventRec[eventLoc-1].eventdat4 || eventRec[eventLoc-1].eventdat4 == 0))
			{
				if (eventRec[eventLoc-1].eventdat != -99)
				{
					enemy[temp].excc = eventRec[eventLoc-1].eventdat;
					enemy[temp].exccw = abs(eventRec[eventLoc-1].eventdat);
					enemy[temp].exccwmax = abs(eventRec[eventLoc-1].eventdat);
					if (eventRec[eventLoc-1].eventdat > 0)
						enemy[temp].exccadd = 1;
					else
						enemy[temp].exccadd = -1;
				}

				if (eventRec[eventLoc-1].eventdat2 != -99)
				{
					enemy[temp].eycc = eventRec[eventLoc-1].eventdat2;
					enemy[temp].eyccw = abs(eventRec[eventLoc-1].eventdat2);
					enemy[temp].eyccwmax = abs(eventRec[eventLoc-1].eventdat2);
					if (eventRec[eventLoc-1].eventdat2 > 0)
						enemy[temp].eyccadd = 1;
					else
						enemy[temp].eyccadd = -1;
				}

				if (eventRec[eventLoc-1].eventdat5 > 0)
				{
					enemy[temp].enemycycle = eventRec[eventLoc-1].eventdat5;
				}
				if (eventRec[eventLoc-1].eventdat6 > 0)
				{
					enemy[temp].ani = eventRec[eventLoc-1].eventdat6;
					enemy[temp].animin = eventRec[eventLoc-1].eventdat5;
					enemy[temp].animax = 0;
					enemy[temp].aniactive = 1;
				}
			}
		}
		break;

	case 21:
		event_name("background3_over");
		background3over = 1;
		break;

	case 22:
		event_name("background3_notover");
		background3over = 0;
		break;

	case 23: /* Sky Enemy on Bottom */
		event_name("new_sky2bottom_enemy");
		JE_createNewEventEnemy(0, 50, 0);
		if (b > 0)
			enemy[b-1].ey = 180 + eventRec[eventLoc-1].eventdat5;
		break;

	case 24: /* Enemy Global Animate */
		event_name("enemy_global_animate");
		for (temp = 0; temp < 100; temp++)
		{
			if (enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				enemy[temp].aniactive = 1;
				enemy[temp].aniwhenfire = 0;
				if (eventRec[eventLoc-1].eventdat2 > 0)
				{
					enemy[temp].enemycycle = eventRec[eventLoc-1].eventdat2;
					enemy[temp].animin = enemy[temp].enemycycle;
				}
				else
				{
					enemy[temp].enemycycle = 0;
				}

				if (eventRec[eventLoc-1].eventdat > 0)
					enemy[temp].ani = eventRec[eventLoc-1].eventdat;

				if (eventRec[eventLoc-1].eventdat3 == 1)
				{
					enemy[temp].animax = enemy[temp].ani;
				}
				else if (eventRec[eventLoc-1].eventdat3 == 2)
				{
					enemy[temp].aniactive = 2;
					enemy[temp].animax = enemy[temp].ani;
					enemy[temp].aniwhenfire = 2;
				}
			}
		}
		break;

	case 25: /* Enemy Global Damage change */
	case 47: // This game, I swear...
		event_name("enemy_global_health_change_legacy");
		int newarmor = (eventRec[eventLoc-1].eventdat == 255) ? ENEMY_INVULNERABLE : eventRec[eventLoc-1].eventdat * 100;
		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
				enemy[temp].ehealth = newarmor;
		}
		break;

	case 26:
		event_name("small_enemy_adjust");
		smallEnemyAdjust = eventRec[eventLoc-1].eventdat;
		break;

	case 27: /* Enemy Global AccelRev */
		event_name("enemy_global_accelrev");
		if (eventRec[eventLoc-1].eventdat3 > 79 && eventRec[eventLoc-1].eventdat3 < 90)
			eventRec[eventLoc-1].eventdat4 = newPL[eventRec[eventLoc-1].eventdat3 - 80];

		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				if (eventRec[eventLoc-1].eventdat != -99)
					enemy[temp].exrev = eventRec[eventLoc-1].eventdat;
				if (eventRec[eventLoc-1].eventdat2 != -99)
					enemy[temp].eyrev = eventRec[eventLoc-1].eventdat2;
				if (eventRec[eventLoc-1].eventdat3 != 0 && eventRec[eventLoc-1].eventdat3 < 17)
					enemy[temp].filter = eventRec[eventLoc-1].eventdat3;
			}
		}
		break;

	case 28:
		event_name("top_enemy_notover");
		topEnemyOver = false;
		break;

	case 29:
		event_name("top_enemy_over");
		topEnemyOver = true;
		break;

	case 30:
		event_name("map_backmove_set");
		map1YDelay = 1;
		map1YDelayMax = 1;
		map2YDelay = 1;
		map2YDelayMax = 1;

		backMove = eventRec[eventLoc-1].eventdat;
		backMove2 = eventRec[eventLoc-1].eventdat2;
		explodeMove = backMove2;
		backMove3 = eventRec[eventLoc-1].eventdat3;
		break;

	case 31: /* Enemy Fire Override */
		event_name("enemy_fire_override");
		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 99 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				enemy[temp].freq[1-1] = eventRec[eventLoc-1].eventdat ;
				enemy[temp].freq[2-1] = eventRec[eventLoc-1].eventdat2;
				enemy[temp].freq[3-1] = eventRec[eventLoc-1].eventdat3;
				for (temp2 = 0; temp2 < 3; temp2++)
				{
					enemy[temp].eshotwait[temp2] = 1;
				}
				if (enemy[temp].launchtype > 0)
				{
					enemy[temp].launchfreq = eventRec[eventLoc-1].eventdat5;
					enemy[temp].launchwait = 1;
				}
			}
		}
		break;

	case 32:  // create enemy
		event_name("new_event_enemy");
		JE_createNewEventEnemy(0, 50, 0);
		if (b > 0)
			enemy[b-1].ey = 190;
		break;

	case 33: /* Enemy From other Enemies */
		event_name("enemy_from_other_enemies");
		if (eventRec[eventLoc-1].eventdat == 512 || eventRec[eventLoc-1].eventdat == 513)
			break; // ignore old data cubes

		if (eventRec[eventLoc-1].eventdat == 534)
			eventRec[eventLoc-1].eventdat = 827;
		for (temp = 0; temp < 100; temp++)
		{
			if (enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
				enemy[temp].enemydie = eventRec[eventLoc-1].eventdat;
		}
		break;

	case 34: /* Start Music Fade */
		event_name("music_fade");
		if (can_play_audio())
		{
			musicFade = true;
			tempVolume = tyrMusicVolume;
		}
		break;

	case 35: /* Play new song */
		event_name("music_play");
		if (can_play_audio())
		{
			play_song(eventRec[eventLoc-1].eventdat - 1);
			set_volume(tyrMusicVolume, fxVolume);
		}
		musicFade = false;
		break;

	case 36:
		event_name("ready_to_end_level");
		readyToEndLevel = true;
		break;

	case 37:
		event_name("level_enemy_frequency");
		levelEnemyFrequency = eventRec[eventLoc-1].eventdat;
		break;

	case 38:
		event_name("jump_screenloc");
		curLoc = eventRec[eventLoc-1].eventdat;
		int new_event_loc = 1;
		for (tempW = 0; tempW < maxEvent; tempW++)
		{
			if (eventRec[tempW].eventtime <= curLoc)
			{
				new_event_loc = tempW+1 - 1;
			}
		}
		eventLoc = new_event_loc;
		break;

	case 39: /* Enemy Global Linknum Change */
		event_name("enemy_global_linknum");
		for (temp = 0; temp < 100; temp++)
		{
			if (enemy[temp].linknum == eventRec[eventLoc-1].eventdat)
				enemy[temp].linknum = eventRec[eventLoc-1].eventdat2;
		}
		break;

	case 40: /* Enemy Continual Damage */
		event_name("enemy_continual_damage");
		enemyContinualDamage = true;
		break;

	case 41:
		event_name("remove_enemies");
		if (eventRec[eventLoc-1].eventdat == 0)
		{
			memset(enemyAvail, 1, sizeof(enemyAvail));
		}
		else
		{
			for (x = 0; x <= 24; x++)
				enemyAvail[x] = 1;
		}
		break;

	case 42:
		event_name("background3_over2");
		background3over = 2;
		break;

	case 43:
		event_name("background2_overset");
		background2over = eventRec[eventLoc-1].eventdat;
		break;

	case 44:
		event_name("filter");
		filterActive       = (eventRec[eventLoc-1].eventdat > 0);
		filterFade         = (eventRec[eventLoc-1].eventdat == 2);
		levelFilter        = eventRec[eventLoc-1].eventdat2;
		levelBrightness    = eventRec[eventLoc-1].eventdat3;
		levelFilterNew     = eventRec[eventLoc-1].eventdat4;
		levelBrightnessChg = eventRec[eventLoc-1].eventdat5;
		filterFadeStart    = (eventRec[eventLoc-1].eventdat6 == 0);
		break;

	case 45: /* arcade-only enemy from other enemies */
		event_name("arcade_enemy_from_other_enemies");
		// Guess what? We're always in arcade mode.
		for (temp = 0; temp < 100; temp++)
		{
			if (enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
				enemy[temp].enemydie = eventRec[eventLoc-1].eventdat;
		}
		break;

	case 46:  // change difficulty
		event_name("difficulty_change");
		if (eventRec[eventLoc-1].eventdat3 != 0)
			damageRate = eventRec[eventLoc-1].eventdat3;
		ARC_RankLevelAdjusts(eventRec[eventLoc-1].eventdat);
		break;

	case 48: /* Background 2 Cannot be Transparent */
		event_name("background2_nottransparent");
		background2notTransparent = true;
		break;

	case 49:
	case 50:
	case 51:
	case 52:
		event_name("new_event_enemy?");
	{
		JE_integer backupData1 = eventRec[eventLoc-1].eventdat;
		JE_integer backupData3 = eventRec[eventLoc-1].eventdat3;
		JE_integer backupData6 = eventRec[eventLoc-1].eventdat6;
		eventRec[eventLoc-1].eventdat = 0;
		eventRec[eventLoc-1].eventdat3 = 0;
		eventRec[eventLoc-1].eventdat6 = 0;
		enemyDat[0].armor = backupData6;
		enemyDat[0].egraphic[1-1] = backupData1;
		switch (eventRec[eventLoc-1].eventtype - 48)
		{
		case 1:
			temp = 25;
			break;
		case 2:
			temp = 0;
			break;
		case 3:
			temp = 50;
			break;
		case 4:
			temp = 75;
			break;
		}
		JE_createNewEventEnemy(0, temp, backupData3);
		eventRec[eventLoc-1].eventdat = backupData1;
		eventRec[eventLoc-1].eventdat3 = backupData3;
		eventRec[eventLoc-1].eventdat6 = backupData6;
		break;		
	}

	case 53:
		event_name("force_events");
		forceEvents = (eventRec[eventLoc-1].eventdat != 99);
		break;

	case 54:
		event_name("event_jump");
		JE_eventJump(eventRec[eventLoc-1].eventdat);
		break;

	case 55: /* Enemy Global AccelRev */
		event_name("enemy_global_accelrev");
		if (eventRec[eventLoc-1].eventdat3 > 79 && eventRec[eventLoc-1].eventdat3 < 90)
			eventRec[eventLoc-1].eventdat4 = newPL[eventRec[eventLoc-1].eventdat3 - 80];

		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				if (eventRec[eventLoc-1].eventdat != -99)
					enemy[temp].xaccel = eventRec[eventLoc-1].eventdat;
				if (eventRec[eventLoc-1].eventdat2 != -99)
					enemy[temp].yaccel = eventRec[eventLoc-1].eventdat2;
			}
		}
		break;

	case 56: /* Ground2 Bottom */
		event_name("new_ground2bottom_enemy");
		JE_createNewEventEnemy(0, 75, 0);
		if (b > 0)
			enemy[b-1].ey = 190;
		break;

	case 57:
		event_name("superenemy_jump");
		superEnemy254Jump = eventRec[eventLoc-1].eventdat;
		break;

	case 58: // T2000
		event_name("t2k_set_enemy_launch");
		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 99 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
				enemy[temp].launchtype = eventRec[eventLoc-1].eventdat;
		}
		break;

	case 59: // T2000
	case 68: // What the hell, Tyrian 2000? (See event 99)
		event_name("t2k_replace_enemy");
		{
			Uint16 eDatI = eventRec[eventLoc-1].eventdat;

			for (temp = 0; temp < 100; temp++)
			{
				if (!(eventRec[eventLoc-1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4))
					continue;

				const int enemy_offset = (enemyDat[eDatI].value > 30000) ? 100 : (temp - (temp % 25));
				b = JE_newEnemy(enemy_offset, eDatI, 0);
				if (b != 0)
				{
					enemy[b-1].ex = enemy[temp].ex;
					enemy[b-1].ey = enemy[temp].ey;
				}

				enemyAvail[temp] = 1;
			}			
		}
		break;

	case 60: /*Assign Special Enemy*/
		event_name("specialenemy_assign");
		for (temp = 0; temp < 100; temp++)
		{
			if (enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				enemy[temp].special = true;
				enemy[temp].flagnum = eventRec[eventLoc-1].eventdat;
				enemy[temp].setto  = (eventRec[eventLoc-1].eventdat2 == 1);
			}
		}
		break;

	case 61:  // if specific flag set to specific value, skip events
		event_name("jump_if_flag_set");
		if (globalFlags[eventRec[eventLoc-1].eventdat-1] == eventRec[eventLoc-1].eventdat2)
			eventLoc += eventRec[eventLoc-1].eventdat3;
		break;

	case 62: /*Play sound effect*/
		event_name("play_sfx");
		soundQueue[3] = eventRec[eventLoc-1].eventdat;
		break;

	case 63:  // skip events if not in 2-player mode
		event_name("jump_if_fullgame");
		break;

	case 64:
		event_name("smoothies");
		smoothies[eventRec[eventLoc-1].eventdat-1] = eventRec[eventLoc-1].eventdat2;
		temp = eventRec[eventLoc-1].eventdat;
		if (temp == 5)
			temp = 3;
		smoothie_data[temp-1] = eventRec[eventLoc-1].eventdat3;

		break;

	case 65:
		event_name("background_3x1");
		background3x1 = (eventRec[eventLoc-1].eventdat == 0);
		break;

	case 66: /*If not on this difficulty level or higher then...*/
		event_name("jump_if_difficulty");
		if (initialDifficulty <= eventRec[eventLoc-1].eventdat)
			eventLoc += eventRec[eventLoc-1].eventdat2;
		break;

	case 67:
		event_name("level_timer");
		levelTimer = (eventRec[eventLoc-1].eventdat == 1);
		levelTimerCountdown = eventRec[eventLoc-1].eventdat3 * 100;
		levelTimerJumpTo   = eventRec[eventLoc-1].eventdat2;
		break;

	case 69:
		event_name("set_invulnerable");
		for (uint i = 0; i < COUNTOF(player); ++i)
			player[i].invulnerable_ticks = eventRec[eventLoc-1].eventdat;
		break;

	case 70:
		event_name("jump_searchfor");
		if (eventRec[eventLoc-1].eventdat2 == 0)
		{  /*1-10*/
			bool found = false;

			for (temp = 1; temp <= 19; temp++)
				found = found || JE_searchFor(temp, NULL);

			if (!found)
				JE_eventJump(eventRec[eventLoc-1].eventdat);
		}
		else if (!JE_searchFor(eventRec[eventLoc-1].eventdat2, NULL)
		         && (eventRec[eventLoc-1].eventdat3 == 0 || !JE_searchFor(eventRec[eventLoc-1].eventdat3, NULL))
		         && (eventRec[eventLoc-1].eventdat4 == 0 || !JE_searchFor(eventRec[eventLoc-1].eventdat4, NULL)))
		{
			JE_eventJump(eventRec[eventLoc-1].eventdat);
		}
		break;

	case 71:
		event_name("jump_???");
		if (((((intptr_t)mapYPos - (intptr_t)&megaData1.mainmap) / sizeof(JE_byte *)) * 2) <= (unsigned)eventRec[eventLoc-1].eventdat2)
		{
			JE_eventJump(eventRec[eventLoc-1].eventdat);
		}
		break;

	case 72:
		event_name("background_3x1b");
		background3x1b = (eventRec[eventLoc-1].eventdat == 1);
		break;

	case 73:
		event_name("sky_enemy_over_all");
		skyEnemyOverAll = (eventRec[eventLoc-1].eventdat == 1);
		break;

	case 74: /* Enemy Global BounceParams */
		event_name("enemy_global_bounceparams");
		for (temp = 0; temp < 100; temp++)
		{
			if (eventRec[eventLoc-1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc-1].eventdat4)
			{
				if (eventRec[eventLoc-1].eventdat5 != -99)
					enemy[temp].xminbounce = eventRec[eventLoc-1].eventdat5;

				if (eventRec[eventLoc-1].eventdat6 != -99)
					enemy[temp].yminbounce = eventRec[eventLoc-1].eventdat6;

				if (eventRec[eventLoc-1].eventdat != -99)
					enemy[temp].xmaxbounce = eventRec[eventLoc-1].eventdat;

				if (eventRec[eventLoc-1].eventdat2 != -99)
					enemy[temp].ymaxbounce = eventRec[eventLoc-1].eventdat2;
			}
		}
		break;

	case 75:;
		event_name("i_have_no_clue_either");
		bool temp_no_clue = false; // TODO: figure out what this is doing

		for (temp = 0; temp < 125; temp++)
		{
			if (enemyAvail[temp] == 0
			    && enemy[temp].eyc == 0
			    && enemy[temp].linknum >= eventRec[eventLoc-1].eventdat
			    && enemy[temp].linknum <= eventRec[eventLoc-1].eventdat2)
			{
				temp_no_clue = true;
			}
		}

		if (temp_no_clue)
		{
			JE_byte enemy_i;
			do
			{
				temp = (mt_rand() % (eventRec[eventLoc-1].eventdat2 + 1 - eventRec[eventLoc-1].eventdat)) + eventRec[eventLoc-1].eventdat;
			}
			while (!(JE_searchFor(temp, &enemy_i) && enemy[enemy_i].eyc == 0));

			newPL[eventRec[eventLoc-1].eventdat3 - 80] = temp;
		}
		else
		{
			newPL[eventRec[eventLoc-1].eventdat3 - 80] = 255;
			if (eventRec[eventLoc-1].eventdat4 > 0)
			{ /*Skip*/
				curLoc = eventRec[eventLoc-1 + eventRec[eventLoc-1].eventdat4].eventtime - 1;
				eventLoc += eventRec[eventLoc-1].eventdat4 - 1;
			}
		}

		break;

	case 76:
		event_name("return_active");
		returnActive = true;
		break;

	case 77:
		event_name("jump_map");
		mapYPos = &megaData1.mainmap[0][0];
		mapYPos += eventRec[eventLoc-1].eventdat / 2;
		if (eventRec[eventLoc-1].eventdat2 > 0)
		{
			mapY2Pos = &megaData2.mainmap[0][0];
			mapY2Pos += eventRec[eventLoc-1].eventdat2 / 2;
		}
		else
		{
			mapY2Pos = &megaData2.mainmap[0][0];
			mapY2Pos += eventRec[eventLoc-1].eventdat / 2;
		}
		break;

	case 78:
		event_name("galaga_shot_freq");
		// Event dummied out
		break;

	case 79:
		event_name("set_boss_bar");
		boss_bar[0].link_num = eventRec[eventLoc-1].eventdat;
		boss_bar[1].link_num = eventRec[eventLoc-1].eventdat2;
		boss_bar[0].color = boss_bar[1].color = 6;
		break;

	case 80:  // skip events if in 2-player mode
		event_name("jump_if_2player");
		if (PL_NumPlayers() == 2)
			eventLoc += eventRec[eventLoc-1].eventdat;
		break;

	case 81: /*WRAP2*/
		event_name("wrap2");
		BKwrap2   = &megaData2.mainmap[0][0];
		BKwrap2   += eventRec[eventLoc-1].eventdat / 2;
		BKwrap2to = &megaData2.mainmap[0][0];
		BKwrap2to += eventRec[eventLoc-1].eventdat2 / 2;
		break;

	case 82: /*Give SPECIAL WEAPON*/
		event_name("give_special_weapon");
		// Note: This is currently commented out because of HOLES.
/*	
		player[0].items.special[player[0].special_mode] = eventRec[eventLoc-1].eventdat;
		player[1].items.special[player[1].special_mode] = eventRec[eventLoc-1].eventdat;
		PL_SwitchSpecial(&player[0], player[0].special_mode, false);
		PL_SwitchSpecial(&player[1], player[1].special_mode, false);
*/
		break;

	case 83: // I don't know why T2K has a separate event for this but after random_explosions nothing surprises me
		event_name("t2k_map_stop");
		stopBackgrounds = true;
		switch (eventRec[eventLoc-1].eventdat)
		{
		case 0:
		case 1:
			stopBackgroundNum = 1;
			break;
		case 2:
			stopBackgroundNum = 2;
			break;
		case 3:
			stopBackgroundNum = 3;
			break;
		}
		break;

	case 84: // T2000 Timed Battle parameters
		event_name("t2k_timed_battle_params");
		break; // Ignored -- no reason to do anything with these

	case 85: // T2000 Timed Battle enemy from other enemies
		event_name("t2k_tb_enemy_from_other_enemies");
		break; // Ignored -- no reason to do anything with these

	case 99:
		event_name("random_explosions");
		// So, random explosions is event 68 in Tyrian 2.1, but 99 in 2000, for some unknown reason.
		// We decided to take the path of patching the 2.1 levels to use this event, instead of
		// doing a whole bunch of nonsense that depended on whether Tyrian 2000 was loaded or not.
		randomExplosions = (eventRec[eventLoc-1].eventdat == 1);
		break;

	case 100: // Sky enemy with normal powerup
	case 101: // Sky enemy with shield powerup
		event_name("new_sky_enemy_powerup");
		JE_createNewEventEnemy(0, 0, 0);
		if (b > 0)
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 100) ? 999 : 533);
		break;

	case 102: // Ground enemy with normal powerup
	case 103: // Ground enemy with shield powerup
		event_name("new_ground_enemy_powerup");
		JE_createNewEventEnemy(0, 25, 0);
		if (b > 0)
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 102) ? 999 : 533);
		break;

	case 104: // Top enemy with normal powerup
	case 105: // Top enemy with shield powerup
		event_name("new_top_enemy_powerup");
		JE_createNewEventEnemy(0, 50, 0);
		if (b > 0)
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 104) ? 999 : 533);
		break;

	case 106: // Ground2 enemy with normal powerup
	case 107: // Ground2 enemy with shield powerup
		event_name("new_ground2_enemy_powerup");
		JE_createNewEventEnemy(0, 75, 0);
		if (b > 0)
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 106) ? 999 : 533);
		break;

	case 108: // Skybottom enemy with normal powerup
	case 109: // Skybottom enemy with shield powerup
		event_name("new_skybottom_enemy_powerup");
		JE_createNewEventEnemy(0, 0, 0);
		if (b > 0)
		{
			enemy[b-1].ey = 190 + eventRec[eventLoc-1].eventdat5;
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 108) ? 999 : 533);
		}
		break;

	case 110: // Groundbottom enemy with normal powerup
	case 111: // Groundbottom enemy with shield powerup
		event_name("new_groundbottom_enemy_powerup");
		JE_createNewEventEnemy(0, 25, 0);
		if (b > 0)
		{
			enemy[b-1].ey = 190 + eventRec[eventLoc-1].eventdat5;
			enemy[b-1].enemydie = ((eventRec[eventLoc-1].eventtype == 110) ? 999 : 533);
		}
		break;

	case 120:; // jump to random
		event_name("jump_to_random");
		JE_word new_time = eventRec[eventLoc-1].eventdat;
		new_time += (eventRec[eventLoc-1].eventdat2 * (mt_rand() % eventRec[eventLoc-1].eventdat3));
		JE_eventJump(new_time);
		break;

	case 254:; // ARC drop help powerup
		event_name("arc_powerup_help");
		uint powerItem = 0;
		if (eventRec[eventLoc-1].eventdat == 1 && PL_Alive(0)
		 && (player[0].items.power_level < (unsigned)eventRec[eventLoc-1].eventdat2))
			powerItem = player[0].port_mode;
		else if (eventRec[eventLoc-1].eventdat == 2 && PL_Alive(1)
		 && (player[1].items.power_level < (unsigned)eventRec[eventLoc-1].eventdat2))
			powerItem = player[1].port_mode;
		else
			break;

		b = JE_newEnemy(100, 999, 0);
		if (b == 0)
			break;

		enemy[b-1].egr[1-1] = 5 + (powerItem + 1) * 2;
		enemy[b-1].evalue = powerItem + 30001;
		enemy[b-1].scoreitem = true;
		enemy[b-1].ex = (eventRec[eventLoc-1].eventdat == 1) ? 40 : 160;
		enemy[b-1].ey = -24;
		enemy[b-1].eyc = 1;

		break;

	case 255: // ARC set hurryup timer
		event_name("arc_hurryup_timer");
		break;

	default:
		event_name("unknown");
		fprintf(stderr, "warning: ignoring unknown event %d\n", eventRec[eventLoc-1].eventtype);
		break;
	}

#undef event_name
#ifdef EVENT_LOGGING
	printf("(%d %d %d %d %d %d)\n", eventRec[prEventLoc-1].eventdat,  eventRec[prEventLoc-1].eventdat2, eventRec[prEventLoc-1].eventdat3,
		eventRec[prEventLoc-1].eventdat4, eventRec[prEventLoc-1].eventdat5, eventRec[prEventLoc-1].eventdat6);
#endif

	eventLoc++;
}

void JE_barX( JE_word x1, JE_word y1, JE_word x2, JE_word y2, JE_byte col )
{
	fill_rectangle_xy(VGAScreen, x1, y1,     x2, y1,     col + 1);
	fill_rectangle_xy(VGAScreen, x1, y1 + 1, x2, y2 - 1, col    );
	fill_rectangle_xy(VGAScreen, x1, y2,     x2, y2,     col - 1);
}

void draw_boss_bar( void )
{
	JE_boolean timerActive = (levelTimer || (hurryUpTimer < 995));
	unsigned int bars = 0;

	for (unsigned int b = 0; b < COUNTOF(boss_bar); b++)
	{
		if (boss_bar[b].link_num == 0)
			continue;
		++bars;

		unsigned int worst_health = ENEMY_INVULNERABLE+1;  // higher than armor max
		for (unsigned int e = 0; e < COUNTOF(enemy); e++)  // find most damaged
		{
			if (enemyAvail[e] != 1 && enemy[e].linknum == boss_bar[b].link_num)
				if (enemy[e].ehealth < worst_health)
					worst_health = enemy[e].ehealth;
		}

		if (worst_health == ENEMY_INVULNERABLE+1 || worst_health == 0)  // boss dead?
			boss_bar[b].link_num = 0;
		else
			boss_bar[b].armor = worst_health / 100;
	}

	// if only one bar left, make it the first one
	if (bars == 1 && boss_bar[0].link_num == 0)
	{
		memcpy(&boss_bar[0], &boss_bar[1], sizeof(boss_bar_t));
		boss_bar[1].link_num = 0;
	}

	for (unsigned int b = 0; b < bars; b++)
	{
		unsigned int x = 157, y = 12;
		if (timerActive) // level timer and boss bar would overlap
			y = 26;
		if (bars == 2)
			x = (b == 0) ? 127 : 187;

		if (boss_bar[b].armor > 254)
			boss_bar[b].armor = 254;
		JE_barX(x - 25, y, x + 25, y + 5, 115);
		JE_barX(x - (boss_bar[b].armor / 10), y, x + (boss_bar[b].armor + 5) / 10, y + 5, 118 + boss_bar[b].color);			

		if (boss_bar[b].color > 0)
			boss_bar[b].color--;
	}
}
