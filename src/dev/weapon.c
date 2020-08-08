/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  dev/weapon.c
/// \brief Weapon visualizer devtool

#include "../opentyr.h"

#ifdef ENABLE_DEVTOOLS

#include "../arcade.h"
#include "../backgrnd.h"
#include "../fonthand.h"
#include "../input.h"
#include "../loudness.h"
#include "../mainint.h"
#include "../nortsong.h"
#include "../palette.h"
#include "../picload.h"
#include "../playdata.h"
#include "../player.h"
#include "../shots.h"
#include "../video.h"
#include "../vga256d.h"

static int useWeapon = 0;
static int selectWeapon = 1, origWeapon = 0;
static bool isSelecting = false;
static bool allowedCancel = false;

void JE_weaponViewFrame( void )
{
	fill_rectangle_xy(VGAScreen, 0, 0, 140, 182, 0);

	/* JE: (* Port Configuration Display *)
	(*    drawportconfigbuttons;*/

	update_and_draw_starfield(VGAScreen, 1);

	// create shots in weapon simulator
	if (PL_ShotRepeat(&player[0], SHOT_NORMAL))
	{
		player_shot_create(1, SHOT_NORMAL, player[0].x, player[0].y, (isSelecting) ? selectWeapon : useWeapon, 1);
	}

	simulate_player_shots();

	blit_sprite(VGAScreenSeg, 0, 0, OPTION_SHAPES, 12); // upgrade interface

	//JE_waitFrameCount();  TODO: didn't do anything?
}

void JE_weaponSimUpdate( void )
{
	JE_weaponViewFrame();
	blit_sprite2x2(VGAScreen, player[0].x - 5, player[0].y - 7, shipShapes, ships[player[0].items.ship].shipgraphic);
}

void JE_initWeaponView( void )
{
	fill_rectangle_xy(VGAScreen, 0, 0, 160, 183, 0);

	player[0].x = player[0].initial_x = 72;
	player[0].y = player[0].initial_y = 110;
	player[0].delta_x_shot_move = 0;
	player[0].delta_y_shot_move = 0;
	player[0].last_x_explosion_follow = 72;
	player[0].last_y_explosion_follow = 110;

	memset(shotAvail, 0, sizeof(shotAvail));

	memset(player[0].shot_repeat, 1, sizeof(player[0].shot_repeat));
	memset(player[0].shot_multi_pos, 0, sizeof(player[0].shot_multi_pos));

	initialize_starfield();
}
typedef struct {
	const char *n;
	int t;
	union {
		JE_byte *as_byte;
		JE_word *as_word;
		JE_shortint *as_siarr;
		JE_byte *as_barr;
		JE_word *as_warr;
	} v;
} awcmenu;
awcmenu awcOpt[] = {
	{"Delay:", 0}, {"AnimFrames:", 1}, {"PiecesPerFire:", 0}, {"NumPieces:", 0},
	{"del:", 3}, {"mvX:", 2}, {"mvY:", 2}, {"x:", 2},
	{"y:", 2}, {"spr:", 4}, {"acY:", 2}, {"acX:", 2},
	{"cir:", 3},
};

void DEV_WeaponCreator( void )
{
	bool fade_in = true;
	size_t shotSize = sizeof(JE_WeaponType) * (num_pWeapons + 2);
	pWeapons = realloc(pWeapons, shotSize);
	useWeapon = ++num_pWeapons;

	// setup options
	awcOpt[ 0].v.as_byte = &pWeapons[useWeapon].shotrepeat;
	awcOpt[ 1].v.as_word = &pWeapons[useWeapon].weapani;
	awcOpt[ 2].v.as_byte = &pWeapons[useWeapon].multi;
	awcOpt[ 3].v.as_byte = &pWeapons[useWeapon].max;
	awcOpt[ 4].v.as_barr = pWeapons[useWeapon].del;
	awcOpt[ 5].v.as_siarr = pWeapons[useWeapon].sx;
	awcOpt[ 6].v.as_siarr = pWeapons[useWeapon].sy;
	awcOpt[ 7].v.as_siarr = pWeapons[useWeapon].bx;
	awcOpt[ 8].v.as_siarr = pWeapons[useWeapon].by;
	awcOpt[ 9].v.as_warr = pWeapons[useWeapon].sg;
	awcOpt[10].v.as_siarr = pWeapons[useWeapon].acceleration;
	awcOpt[11].v.as_siarr = pWeapons[useWeapon].accelerationx;
	awcOpt[12].v.as_barr = pWeapons[useWeapon].circlesize;

	JE_byte menu_ud = 0, menu_lr = 0;

	skip_header_draw = true;

	player[0].items.ship = 9;

	JE_initWeaponView();

	play_song(11);

	isSelecting = true;

	for (; ; )
	{
		setjasondelay(2);

		JE_loadPic(VGAScreen, 1, true);

		JE_weaponSimUpdate();

		for (int i = 0; i < 13; ++i)
		{
			if (awcOpt[i].t > 1)
				JE_outTextAndDarken(VGAScreen, 178 - JE_textWidth(awcOpt[i].n, TINY_FONT), 40 + (i * 8), awcOpt[i].n, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
			else
				JE_outTextAndDarken(VGAScreen, 238 - JE_textWidth(awcOpt[i].n, TINY_FONT), 40 + (i * 8), awcOpt[i].n, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);

			switch (awcOpt[i].t)
			{
			case 0:
				sprintf(tmpBuf.s, "%hhu", *awcOpt[i].v.as_byte);
				JE_outTextAndDarken(VGAScreen, 240, 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
				break;
			case 1:
				sprintf(tmpBuf.s, "%hu", *awcOpt[i].v.as_word);
				JE_outTextAndDarken(VGAScreen, 240, 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
				break;
			case 2:
				for (int j = 0; j < pWeapons[useWeapon].max; ++j)
				{
					sprintf(tmpBuf.s, "%hhd", awcOpt[i].v.as_siarr[j]);
					JE_outTextAndDarken(VGAScreen, 180 + (17 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			case 3:
				for (int j = 0; j < pWeapons[useWeapon].max; ++j)
				{
					sprintf(tmpBuf.s, "%hhu", awcOpt[i].v.as_barr[j]);
					JE_outTextAndDarken(VGAScreen, 180 + (17 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			case 4:
				for (int j = 0; j < pWeapons[useWeapon].max; ++j)
				{
					sprintf(tmpBuf.s, "%hu", awcOpt[i].v.as_warr[j]);
					JE_outTextAndDarken(VGAScreen, 180 + (17 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			}
		}

		if (!isSelecting)
		{	
			blit_sprite(VGAScreenSeg, 24, 149, OPTION_SHAPES, 13);
			blit_sprite(VGAScreenSeg, 119, 149, OPTION_SHAPES, 14);
			JE_outTextAndDarken(VGAScreen, 10, 188, "~P1 Joystick~ moves, ~P2 Joystick~ changes settings.", 1, 2, TINY_FONT);
		}
		else
			JE_outTextAndDarken(VGAScreen, 10, 188, "Select weapon. ~Service~ exits. ~Service coin~ changes music.", 1, 2, TINY_FONT);
		JE_dString(VGAScreen, 74 + JE_fontCenter("Shot Editor", FONT_SHAPES), 10, "Shot Editor", FONT_SHAPES);
		snprintf(tmpBuf.s, sizeof(tmpBuf.s), "Shot ~%d~", selectWeapon);
		JE_textShade(VGAScreen, 56, 173, tmpBuf.s, 1, 2, DARKEN);

		if (fade_in)
		{
			fade_palette(colors, 10, 0, 255);
			fade_in = false;
		}

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		uint button = 0;
		int move = 1;
		if (isSelecting)
		{
			while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
			{
				switch (button++)
				{
				case INPUT_P1_FIRE:
					JE_playSampleNum(S_SELECT);
					memcpy(&pWeapons[useWeapon], &pWeapons[selectWeapon], sizeof(JE_WeaponType));
					player[0].shot_multi_pos[SHOT_NORMAL] = 0;
					player[0].shot_repeat[SHOT_NORMAL] = 0;
					if (menu_lr >= pWeapons[useWeapon].max)
						menu_lr = 0;
					isSelecting = false;
					break;

				case INPUT_P1_SKICK:
					JE_playSampleNum(S_SPRING);
					if (allowedCancel)
					{
						player[0].shot_multi_pos[SHOT_NORMAL] = 0;
						player[0].shot_repeat[SHOT_NORMAL] = 0;
						selectWeapon = origWeapon;
						isSelecting = false;
					}
					break;

				case INPUT_P1_UP:
					move = 100;
					// fall through
				case INPUT_P1_RIGHT:
					JE_playSampleNum(S_CURSOR);
					player[0].shot_multi_pos[SHOT_NORMAL] = 0;
					player[0].shot_repeat[SHOT_NORMAL] = 0;
					if ((selectWeapon += move) > (int)num_pWeapons - 1)
						selectWeapon -= num_pWeapons - 1;
					break;
				case INPUT_P1_DOWN:
					move = 100;
					// fall through
				case INPUT_P1_LEFT:
					JE_playSampleNum(S_CURSOR);
					player[0].shot_multi_pos[SHOT_NORMAL] = 0;
					player[0].shot_repeat[SHOT_NORMAL] = 0;
					if ((selectWeapon -= move) <= 0)
						selectWeapon += num_pWeapons - 1;
					break;

				case INPUT_SERVICE_COIN:
					play_song((song_playing + 1) % MUSIC_NUM);
					break;
				case INPUT_SERVICE_ENTER:
					JE_playSampleNum(S_SPRING);
					fade_black(10);
					JE_tyrianHalt(0);
				default:
					break;
				}
			}
		}
		else while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			switch (button++)
			{
			case INPUT_P1_FIRE:
				JE_playSampleNum(S_SELECT);
				isSelecting = true;
				origWeapon = selectWeapon;
				allowedCancel = true;
				player[0].shot_multi_pos[SHOT_NORMAL] = 0;
				player[0].shot_repeat[SHOT_NORMAL] = 0;
				break;

			case INPUT_P1_UP:
				JE_playSampleNum(S_CURSOR);
				if (--menu_ud == 255)
					menu_ud = 12;
				break;
			case INPUT_P1_DOWN:
				JE_playSampleNum(S_CURSOR);
				if (++menu_ud == 13)
					menu_ud = 0;
				break;
			case INPUT_P1_LEFT:
				JE_playSampleNum(S_CURSOR);
				if (awcOpt[menu_ud].t > 1 && --menu_lr == 255)
					menu_lr = pWeapons[useWeapon].max - 1;
				break;
			case INPUT_P1_RIGHT:
				JE_playSampleNum(S_CURSOR);
				if (awcOpt[menu_ud].t > 1 && ++menu_lr == pWeapons[useWeapon].max)
					menu_lr = 0;
				break;

			case INPUT_P2_LEFT:
				JE_playSampleNum(S_CLICK);
				player[0].shot_multi_pos[SHOT_NORMAL] = 0;
				player[0].shot_repeat[SHOT_NORMAL] = 0;
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					--(*awcOpt[menu_ud].v.as_byte);
					if (pWeapons[useWeapon].max > 8 || pWeapons[useWeapon].max < 1)
						pWeapons[useWeapon].max = 8;
					if (pWeapons[useWeapon].multi > pWeapons[useWeapon].max || pWeapons[useWeapon].multi < 1)
						pWeapons[useWeapon].multi = pWeapons[useWeapon].max;
					if (menu_lr >= pWeapons[useWeapon].max)
						menu_lr = 0;
					break;
				case 1:
					--(*awcOpt[menu_ud].v.as_word);
					break;
				case 2:
					--(awcOpt[menu_ud].v.as_siarr[menu_lr]);
					break;
				case 3:
					--(awcOpt[menu_ud].v.as_barr[menu_lr]);
					break;
				case 4:
					--(awcOpt[menu_ud].v.as_warr[menu_lr]);
					break;
				}
				break;
			case INPUT_P2_RIGHT:
				JE_playSampleNum(S_CLICK);
				player[0].shot_multi_pos[SHOT_NORMAL] = 0;
				player[0].shot_repeat[SHOT_NORMAL] = 0;
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					++(*awcOpt[menu_ud].v.as_byte);
					if (pWeapons[useWeapon].max > 8)
						pWeapons[useWeapon].max = 1;
					if (pWeapons[useWeapon].multi > pWeapons[useWeapon].max)
						pWeapons[useWeapon].multi = 1;
					if (menu_lr >= pWeapons[useWeapon].max)
						menu_lr = 0;
					break;
				case 1:
					++(*awcOpt[menu_ud].v.as_word);
					break;
				case 2:
					++(awcOpt[menu_ud].v.as_siarr[menu_lr]);
					break;
				case 3:
					++(awcOpt[menu_ud].v.as_barr[menu_lr]);
					break;
				case 4:
					++(awcOpt[menu_ud].v.as_warr[menu_lr]);
					break;
				}
				break;
			case INPUT_P2_UP:
				JE_playSampleNum(S_SELECT);
				player[0].shot_multi_pos[SHOT_NORMAL] = 0;
				player[0].shot_repeat[SHOT_NORMAL] = 0;
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					if (menu_ud == 0)
						(*awcOpt[menu_ud].v.as_byte) -= 32;
					break;
				case 1:
					// no-op
					//(*awcOpt[menu_ud].v.as_word) -= ;
					break;
				case 2:
					(awcOpt[menu_ud].v.as_siarr[menu_lr]) *= -1;
					break;
				case 3:
					(awcOpt[menu_ud].v.as_barr[menu_lr]) -= 20;
					break;
				case 4:
					(awcOpt[menu_ud].v.as_warr[menu_lr]) -= 100;
					break;
				}
				break;
			case INPUT_P2_DOWN:
				JE_playSampleNum(S_SELECT);
				player[0].shot_multi_pos[SHOT_NORMAL] = 0;
				player[0].shot_repeat[SHOT_NORMAL] = 0;
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					if (menu_ud == 0)
						(*awcOpt[menu_ud].v.as_byte) += 32;
					break;
				case 1:
					// no-op
					//(*awcOpt[menu_ud].v.as_word) -= ;
					break;
				case 2:
					(awcOpt[menu_ud].v.as_siarr[menu_lr]) *= -1;
					break;
				case 3:
					(awcOpt[menu_ud].v.as_barr[menu_lr]) += 20;
					break;
				case 4:
					(awcOpt[menu_ud].v.as_warr[menu_lr]) += 100;
					break;
				}
				break;


			case INPUT_SERVICE_COIN:
				play_song((song_playing + 1) % MUSIC_NUM);
				break;
			case INPUT_SERVICE_ENTER:
				JE_playSampleNum(S_SPRING);
				fade_black(10);
				JE_tyrianHalt(0);
			default:
				break;
			}
		}
	}
}

#endif /* ENABLE_DEVTOOLS */
