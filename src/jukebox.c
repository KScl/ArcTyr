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
#include "font.h"
#include "input.h"
#include "jukebox.h"
#include "lds_play.h"
#include "loudness.h"
#include "mtrand.h"
#include "nortsong.h"
#include "opentyr.h"
#include "palette.h"
#include "sprite.h"
#include "vga_palette.h"
#include "video.h"

#include <stdio.h>

// 
// Starlib
// Previously in its own file, merged with the jukebox because this is its only use
//

typedef struct 
{
	JE_integer spX, spY, spZ;
	JE_integer lastX, lastY;
} JE_StarType;

#define starlib_MAX_STARS 1000
#define MAX_TYPES 14

static JE_StarType star[starlib_MAX_STARS];

static const JE_integer starlib_speed = 2;
static const JE_byte pColor = 32;

static JE_word stepCounter, changeTime;
static JE_byte setup = 0;

/* JE: new sprite pointer */
static JE_real nsp, nspVarInc, nspVarVarInc;
static JE_word nsp2;
static JE_shortint nspVar2Inc;

static int tempX, tempY; 

static void JE_newStar( JE_integer *newX, JE_integer *newY, JE_integer *newZ )
{
	*newZ = 500;

	if (setup == 0)
	{
		*newX = (mt_rand() % 64000) - 32000;
		*newY = (mt_rand() % 40000) - 20000;
	} else {
		nsp = nsp + nspVarInc; /* YKS: < lol */
		if (nsp > 48000.0f)
			nsp -= 48000.0f;
		switch (setup)
		{
			case 1:
				*newX = (int)(sinf(nsp / 30) * 20000);
				*newY = (mt_rand() % 40000) - 20000;
				break;
			case 2:
				*newX = (int)(cosf(nsp) * 20000);
				*newY = (int)(sinf(nsp) * 20000);
				break;
			case 3:
				*newX = (int)(cosf(nsp * 15) * 100) * ((int)(nsp / 6) % 200);
				*newY = (int)(sinf(nsp * 15) * 100) * ((int)(nsp / 6) % 200);
				break;
			case 4:
				*newX = (int)(sinf(nsp / 60) * 20000);
				*newY = (int)(cosf(nsp) * (int)(sinf(nsp / 200) * 300) * 100);
				break;
			case 5:
				*newX = (int)(sinf(nsp / 2) * 20000);
				*newY = (int)(cosf(nsp) * (int)(sinf(nsp / 200) * 300) * 100);
				break;
			case 6:
				*newX = (int)(sinf(nsp) * 40000);
				*newY = (int)(cosf(nsp) * 20000);
				break;
			case 8:
				*newX = (int)(sinf(nsp / 2) * 40000);
				*newY = (int)(cosf(nsp) * 20000);
				break;
			case 7:
				*newX = mt_rand() % 65535;
				if ((mt_rand() % 2) == 0)
				{
					*newY = (int)(cosf(nsp / 80) * 10000) + 15000;
				} else {
					*newY = 50000 - (int)(cosf(nsp / 80) * 13000);
				}
				break;
			case 9:
				nsp2 += nspVar2Inc;
				if ((nsp2 == 65535) || (nsp2 == 0))
				{
					nspVar2Inc = -nspVar2Inc;
				}
				*newX = (int)(cosf(sinf(nsp2 / 10.0f) + (nsp / 500)) * 32000);
				*newY = (int)(sinf(cosf(nsp2 / 10.0f) + (nsp / 500)) * 30000);
				break;
			case 10:
				nsp2 += nspVar2Inc;
				if ((nsp2 == 65535) || (nsp2 == 0))
				{
					nspVar2Inc = -nspVar2Inc;
				}
				*newX = (int)(cosf(sinf(nsp2 / 5.0f) + (nsp / 100)) * 32000);
				*newY = (int)(sinf(cosf(nsp2 / 5.0f) + (nsp / 100)) * 30000);
				break;;
			case 11:
				nsp2 += nspVar2Inc;
				if ((nsp2 == 65535) || (nsp2 == 0))
				{
					nspVar2Inc = -nspVar2Inc;
				}
				*newX = (int)(cosf(sinf(nsp2 / 1000.0f) + (nsp / 2)) * 32000);
				*newY = (int)(sinf(cosf(nsp2 / 1000.0f) + (nsp / 2)) * 30000);
				break;
			case 12:
				if (nsp != 0)
				{
					nsp2 += nspVar2Inc;
					if ((nsp2 == 65535) || (nsp2 == 0))
					{
						nspVar2Inc = -nspVar2Inc;
					}
					*newX = (int)(cosf(sinf(nsp2 / 2.0f) / (sqrtf(fabsf(nsp)) / 10.0f + 1) + (nsp2 / 100.0f)) * 32000);
					*newY = (int)(sinf(cosf(nsp2 / 2.0f) / (sqrtf(fabsf(nsp)) / 10.0f + 1) + (nsp2 / 100.0f)) * 30000);
				}
				break;
			case 13:
				if (nsp != 0)
				{
					nsp2 += nspVar2Inc;
					if ((nsp2 == 65535) || (nsp2 == 0))
					{
						nspVar2Inc = -nspVar2Inc;
					}
					*newX = (int)(cosf(sinf(nsp2 / 10.0f) / 2 + (nsp / 20)) * 32000);
					*newY = (int)(sinf(sinf(nsp2 / 11.0f) / 2 + (nsp / 20)) * 30000);
				}
				break;
			case 14:
				nsp2 += nspVar2Inc;
				*newX = (int)((sinf(nsp) + cosf(nsp2 / 1000.0f) * 3) * 12000);
				*newY = (int)(cosf(nsp) * 10000) + nsp2;
				break;
		}
	}
}

static void JE_resetValues( void )
{
	nsp2 = 1;
	nspVar2Inc = 1;
	nsp = 0;
	nspVarInc = 0.1f;
	nspVarVarInc = 0.0001f;
}

static void JE_changeSetup( void )
{
	stepCounter = 0;
	changeTime = (mt_rand() % 667) + 333;

	setup = mt_rand() % (MAX_TYPES + 1);

	if (nspVarInc > 2.2f || setup == 1)
		nspVarInc = 0.1f;
}

static void JE_starlib_init( void )
{
	static JE_boolean initialized = false;

	if (!initialized)
	{
		initialized = true;

		/* RANDOMIZE; */
		for (int x = 0; x < starlib_MAX_STARS; x++)
		{
			star[x].spX = (mt_rand() % 64000) - 32000;
			star[x].spY = (mt_rand() % 40000) - 20000;
			star[x].spZ = x+1;
		}

		changeTime = 100;
	}

	JE_resetValues();
	JE_changeSetup();
}

static void JE_starlib_main( void )
{
	int off;
	JE_word i;
	JE_integer tempZ;
	JE_byte tempCol;
	JE_StarType *stars;
	Uint8 *surf;

	for(stars = star, i = starlib_MAX_STARS; i > 0; stars++, i--)
	{
		/* Make a pointer to the screen... */
		surf = VGAScreen->pixels;

		/* Calculate the offset to where we wish to draw */
		off = (stars->lastX)+(stars->lastY)*320;


		/* We don't want trails in our star field.  Erase the old graphic */
		if (off >= 640 && off < (320*200)-640)
		{
			surf[off] = 0; /* Shade Level 0 */

			surf[off-1] = 0; /* Shade Level 1, 2 */
			surf[off+1] = 0;
			surf[off-2] = 0;
			surf[off+2] = 0;

			surf[off-320] = 0;
			surf[off+320] = 0;
			surf[off-640] = 0;
			surf[off+640] = 0;
		}

		/* Move star */
		tempZ = stars->spZ;
		tempX = (stars->spX / tempZ) + 160;
		tempY = (stars->spY / tempZ) + 100;
		tempZ -=  starlib_speed;


		/* If star is out of range, make a new one */
		if (tempZ <=  0 ||
		    tempY ==  0 || tempY > 198 ||
		    tempX > 318 || tempX <   1)
		{
			JE_newStar(&stars->spX, &stars->spY, &stars->spZ);
		}
		else /* Otherwise, update & draw it */
		{
			stars->lastX = tempX;
			stars->lastY = tempY;
			stars->spZ = tempZ;

			off = tempX+tempY*320;

			// was unreachable code
			//if (grayB)
			//{
			//	tempCol = tempZ >> 1;
			//} 
			//else 
			//{
			tempCol = pColor+((tempZ >> 4) & 31);
			//}

			/* Draw the pixel! */
			if (off >= 640 && off < (320*200)-640)
			{
				surf[off] = tempCol;

				tempCol += 72;
				surf[off-1] = tempCol;
				surf[off+1] = tempCol;
				surf[off-320] = tempCol;
				surf[off+320] = tempCol;

				tempCol += 72;
				surf[off-2] = tempCol;
				surf[off+2] = tempCol;
				surf[off-640] = tempCol;
				surf[off+640] = tempCol;
			}
		}
	}

	stepCounter++;
	if (stepCounter > changeTime)
		JE_changeSetup();

	if ((mt_rand() % 1000) == 1)
	{
		nspVarVarInc = mt_rand_1() * 0.01f - 0.005f;
	}

	nspVarInc += nspVarVarInc;
}


//
// Jukebox
//

void jukebox( void )
{
	uint new_time, enter_time = SDL_GetTicks();

	bool trigger_quit = false,  // true when user wants to quit
	     quitting = false;  // true while quitting
	
	Uint8 fx_num = 0;

	int palette_fade_steps = 15;

	int diff[256][3];
	init_step_fade_palette(diff, vga_palette, 0, 255);

	JE_starlib_init();

	set_volume(tyrMusicVolume, fxVolume);

	for (; ; )
	{
		setdelay(1);

		SDL_FillRect(VGAScreenSeg, NULL, 0);

		// starlib input needs to be rewritten
		JE_starlib_main();

		snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%s %d", soundTitle[fx_num], fx_num + 1);
		draw_font_hv(VGAScreen, 315, 189, tmpBuf.l, small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 315, 191, tmpBuf.l, small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 314, 190, tmpBuf.l, small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 316, 190, tmpBuf.l, small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 315, 190, tmpBuf.l, small_font, right_aligned, 1,  4);

		snprintf(tmpBuf.l, sizeof(tmpBuf.l), "%d %s", song_playing + 1, musicTitle[song_playing]);
		draw_font_hv(VGAScreen,   5, 189, tmpBuf.l, small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   5, 191, tmpBuf.l, small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   4, 190, tmpBuf.l, small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   6, 190, tmpBuf.l, small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   5, 190, tmpBuf.l, small_font, left_aligned,  1,  4);

		new_time = SDL_GetTicks() - enter_time;
		if (new_time < 2000)
		{
			int brightness = (new_time % 500 < 250) ? (new_time % 250 / 50) : 5 - ((new_time % 250) / 50);
			draw_font_hv(VGAScreen, 160, 179, "Service button exits.", small_font, centered,      1, -6);
			draw_font_hv(VGAScreen, 160, 181, "Service button exits.", small_font, centered,      1, -6);
			draw_font_hv(VGAScreen, 159, 180, "Service button exits.", small_font, centered,      1, -6);
			draw_font_hv(VGAScreen, 161, 180, "Service button exits.", small_font, centered,      1, -6);
			draw_font_hv(VGAScreen, 160, 180, "Service button exits.", small_font, centered,      1, brightness);		
		}

		draw_font_hv(VGAScreen, 315, 179, "SFX :P2",               small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 315, 181, "SFX :P2",               small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 314, 180, "SFX :P2",               small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 316, 180, "SFX :P2",               small_font, right_aligned, 1, -6);
		draw_font_hv(VGAScreen, 315, 180, "SFX :P2",               small_font, right_aligned, 0, -3);

		draw_font_hv(VGAScreen,   5, 179, "P1: Music",             small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   5, 181, "P1: Music",             small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   4, 180, "P1: Music",             small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   6, 180, "P1: Music",             small_font, left_aligned,  1, -6);
		draw_font_hv(VGAScreen,   5, 180, "P1: Music",             small_font, left_aligned,  0, -6);

		if (palette_fade_steps > 0)
			step_fade_palette(diff, palette_fade_steps--, 0, 255);
		
		JE_showVGA();
		wait_delay();

		I_checkButtons();
		uint button = INPUT_P1_UP;
		while (I_inputForMenu(&button, INPUT_SERVICE_ENTER))
		{
			switch (button++)
			{
			case INPUT_SERVICE_ENTER: // quit jukebox
				trigger_quit = true;
				break;

			case INPUT_P2_UP:
			case INPUT_P2_LEFT:
				do { --fx_num; } while (!digiFx[fx_num]);
				break;
			case INPUT_P2_RIGHT:
			case INPUT_P2_DOWN:
				do { ++fx_num; } while (!digiFx[fx_num]);
				break;
			case INPUT_P2_FIRE:
				JE_playSampleNum(fx_num + 1);
				break;

			case INPUT_P1_UP:
			case INPUT_P1_LEFT:
				play_song((song_playing > 0 ? song_playing : MUSIC_NUM) - 1);
				break;
			case INPUT_P1_RIGHT:
			case INPUT_P1_DOWN:
				play_song((song_playing + 1) % MUSIC_NUM);
				break;
			case INPUT_P1_MODE: // stop song
				stop_song();
				break;
			case INPUT_P1_FIRE: // restart song
				restart_song();
				break;

			default:
				break;
			}
		}
		
		// user wants to quit, start fade-out
		if (trigger_quit && !quitting)
		{
			palette_fade_steps = 15;
			
			SDL_Color black = { 0, 0, 0 };
			init_step_fade_solid(diff, black, 0, 255);
			
			quitting = true;
		}
		
		// if fade-out finished, we can finally quit
		if (quitting && palette_fade_steps == 0)
			break;
	}

	set_volume(tyrMusicVolume, fxVolume);
}

