/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  devextra.c
/// \brief Extra menus and functions to aide in development

#include "../arcade.h"
#include "../arcserv.h"
#include "../backgrnd.h"
#include "../config.h"
#include "../episodes.h"
#include "../file.h"
#include "../fonthand.h"
#include "../helptext.h"
#include "../input.h"
#include "../loudness.h"
#include "../mainint.h"
#include "../nortsong.h"
#include "../opentyr.h"
#include "../palette.h"
#include "../picload.h"
#include "../player.h"
#include "../shots.h"
#include "../sndmast.h"
#include "../video.h"
#include "../vga256d.h"

#include "../mtrand.h"

#include <time.h>

void JE_weaponViewFrame( void )
{
	fill_rectangle_xy(VGAScreen, 0, 0, 159, 182, 0);

	/* JE: (* Port Configuration Display *)
	(*    drawportconfigbuttons;*/

	update_and_draw_starfield(VGAScreen, 1);

	// create shots in weapon simulator
	if (PL_ShotRepeat(&player[0], FRONT_WEAPON))
	{
		b = player_shot_create(1, FRONT_WEAPON, player[0].x, player[0].y, player[0].x, player[0].y, 1, 1);
	}

	simulate_player_shots();

	//blit_sprite(VGAScreenSeg, 0, 0, OPTION_SHAPES, 12); // upgrade interface

	//JE_waitFrameCount();  TODO: didn't do anything?
}

void JE_weaponSimUpdate( void )
{
	JE_weaponViewFrame();
	blit_sprite2x2(VGAScreen, player[0].x - 5, player[0].y - 7, shapes9, ships[player[0].items.ship].shipgraphic);
}

void JE_initWeaponView( void )
{
	fill_rectangle_xy(VGAScreen, 0, 0, 160, 183, 0);

	player[0].x = 72;
	player[0].y = 110;
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
	{"rep:", 0}, {"ani:", 1}, {"mul:", 0}, {"max:", 0},
	{"-dl:", 3}, {"-sx:", 2}, {"-sy:", 2}, {"-bx:", 2},
	{"-by:", 2}, {"-sg:", 4}, {"-ay:", 2}, {"-ax:", 2},
	{"-cr:", 3},
};

void DEV_WeaponCreator( uint start_weap )
{
	if (start_weap != 1)
		memcpy(&pWeapons[1], &pWeapons[start_weap], sizeof(JE_WeaponType));

	// setup options
	awcOpt[ 0].v.as_byte = &pWeapons[1].shotrepeat;
	awcOpt[ 1].v.as_word = &pWeapons[1].weapani;
	awcOpt[ 2].v.as_byte = &pWeapons[1].multi;
	awcOpt[ 3].v.as_byte = &pWeapons[1].max;
	awcOpt[ 4].v.as_barr = pWeapons[1].del;
	awcOpt[ 5].v.as_siarr = pWeapons[1].sx;
	awcOpt[ 6].v.as_siarr = pWeapons[1].sy;
	awcOpt[ 7].v.as_siarr = pWeapons[1].bx;
	awcOpt[ 8].v.as_siarr = pWeapons[1].by;
	awcOpt[ 9].v.as_warr = pWeapons[1].sg;
	awcOpt[10].v.as_siarr = pWeapons[1].acceleration;
	awcOpt[11].v.as_siarr = pWeapons[1].accelerationx;
	awcOpt[12].v.as_barr = pWeapons[1].circlesize;

	bool fade_in = true;
	JE_byte menu_ud = 0, menu_lr = 0;

	skip_header_draw = true;

	player[0].items.ship = 9;

	JE_initWeaponView();

	play_song(11);

	for (; ; )
	{
		setjasondelay(2);

		JE_loadPic(VGAScreen, 1, true);

		JE_weaponSimUpdate();

		for (int i = 0; i < 13; ++i)
		{
			JE_outTextAndDarken(VGAScreen, 170, 40 + (i * 8), awcOpt[i].n, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
			switch (awcOpt[i].t)
			{
			case 0:
				sprintf(tmpBuf.s, "%hhu", *awcOpt[i].v.as_byte);
				JE_outTextAndDarken(VGAScreen, 200, 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
				break;
			case 1:
				sprintf(tmpBuf.s, "%hu", *awcOpt[i].v.as_word);
				JE_outTextAndDarken(VGAScreen, 200, 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i) ? 4 : 0, TINY_FONT);
				break;
			case 2:
				for (int j = 0; j < pWeapons[1].max; ++j)
				{
					sprintf(tmpBuf.s, "%hhd", awcOpt[i].v.as_siarr[j]);
					JE_outTextAndDarken(VGAScreen, 192 + (16 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			case 3:
				for (int j = 0; j < pWeapons[1].max; ++j)
				{
					sprintf(tmpBuf.s, "%hhu", awcOpt[i].v.as_barr[j]);
					JE_outTextAndDarken(VGAScreen, 192 + (16 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			case 4:
				for (int j = 0; j < pWeapons[1].max; ++j)
				{
					sprintf(tmpBuf.s, "%hu", awcOpt[i].v.as_warr[j]);
					JE_outTextAndDarken(VGAScreen, 192 + (16 * j), 40 + (i * 8), tmpBuf.s, 15, (menu_ud == i && menu_lr == j) ? 4 : 0, TINY_FONT);
				}
				break;
			}
		}
		JE_outTextAndDarken(VGAScreen, 10, 188, "~Service button~ exits. ~Service coin~ changes music.", 1, 2, TINY_FONT);

		if (fade_in)
		{
			fade_palette(colors, 10, 0, 255);
			fade_in = false;
		}

		JE_showVGA();
		wait_delay();

		I_checkButtons();

		uint button = 0;
		while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			switch (button++)
			{
			case INPUT_P1_FIRE:
				JE_playSampleNum(S_SELECT);
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
					menu_lr = pWeapons[1].max - 1;
				break;
			case INPUT_P1_RIGHT:
				JE_playSampleNum(S_CURSOR);
				if (awcOpt[menu_ud].t > 1 && ++menu_lr == pWeapons[1].max)
					menu_lr = 0;
				break;

			case INPUT_P2_LEFT:
				JE_playSampleNum(S_CLICK);
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					--(*awcOpt[menu_ud].v.as_byte);
					if (pWeapons[1].max > 8 || pWeapons[1].max < 1)
						pWeapons[1].max = 8;
					if (pWeapons[1].multi > pWeapons[1].max || pWeapons[1].multi < 1)
						pWeapons[1].multi = pWeapons[1].max;
					if (menu_lr >= pWeapons[1].max)
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
				switch(awcOpt[menu_ud].t)
				{
				case 0:
					++(*awcOpt[menu_ud].v.as_byte);
					if (pWeapons[1].max > 8)
						pWeapons[1].max = 1;
					if (pWeapons[1].multi > pWeapons[1].max)
						pWeapons[1].multi = 1;
					if (menu_lr >= pWeapons[1].max)
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
					(awcOpt[menu_ud].v.as_warr[menu_lr]) -= 500;
					break;
				}
				break;
			case INPUT_P2_DOWN:
				JE_playSampleNum(S_SELECT);
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
					(awcOpt[menu_ud].v.as_warr[menu_lr]) += 500;
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
