/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  opentyr.c
/// \brief Main loop, other source-wide important variables

#include "arcade.h"
#include "config.h"
#include "episodes.h"
#include "file.h"
#include "font.h"
#include "helptext.h"
#include "input.h"
#include "loudness.h"
#include "mainint.h"
#include "menus.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"
#include "picload.h"
#include "playdata.h"
#include "sprite.h"
#include "tyrian2.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"
#include "video/scaler.h"

#include "lib/mtrand.h"

#include "arcade/service.h"

#ifdef ENABLE_DEVTOOLS
#include "dev/demo.h"
#include "dev/weapon.h"
#endif

#include "SDL.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *opentyrian_str = "ArcTyr",
           *opentyrian_version = "unknown", // TODO replace with git
           *opentyrian_date = __DATE__,
           *opentyrian_time = __TIME__;

TempStringBuffer tmpBuf = {"", ""};

JE_byte isInGame = false;

// T2000
bool tyrian2000detected = false;

void opentyrian_menu( void )
{
	STUB();
/*
	typedef enum
	{
		MENU_ABOUT = 0,
		MENU_FULLSCREEN,
		MENU_SCALER,
		// MENU_DESTRUCT,
		MENU_JUKEBOX,
		MENU_RETURN,
		MenuOptions_MAX
	} MenuOptions;

	static const char *menu_items[] =
	{
		"About OpenTyrian",
		"Toggle Fullscreen",
		"Scaler: None",
		// "Play Destruct",
		"Jukebox",
		"Return to Main Menu",
	};
	bool menu_items_disabled[] =
	{
		false,
		!can_init_any_scaler(false) || !can_init_any_scaler(true),
		false,
		// false,
		false,
		false,
	};
	
	assert(COUNTOF(menu_items) == MenuOptions_MAX);
	assert(COUNTOF(menu_items_disabled) == MenuOptions_MAX);

	fade_black(10);
	JE_loadPic(VGAScreen, 13, false);

	draw_font_hv(VGAScreen, VGAScreen->w / 2, 5, opentyrian_str, large_font, centered, 15, -3);

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	JE_showVGA();

	play_song(36); // A Field for Mag

	MenuOptions sel = 0;

	uint temp_scaler = scaler;

	bool fade_in = true, quit = false;
	do
	{
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (MenuOptions i = 0; i < MenuOptions_MAX; i++)
		{
			const char *text = menu_items[i];
			char buffer[100];

			if (i == MENU_SCALER)
			{
				snprintf(buffer, sizeof(buffer), "Scaler: %s", scalers[temp_scaler].name);
				text = buffer;
			}

			int y = i != MENU_RETURN ? i * 16 + 32 : 118;
			draw_font_hv(VGAScreen, VGAScreen->w / 2, y, text, normal_font, centered, 15, menu_items_disabled[i] ? -8 : i != sel ? -4 : -2);
		}

		JE_showVGA();

		if (fade_in)
		{
			fade_in = false;
			fade_palette(colors, 20, 0, 255);
			//wait_noinput(true, false, false);
		}

		tempW = 0;

		if (newkey)
		{
			switch (lastkey_sym)
			{
			case SDLK_UP:
				do
				{
					if (sel-- == 0)
						sel = MenuOptions_MAX - 1;
				}
				while (menu_items_disabled[sel]);
				
				JE_playSampleNum(S_CURSOR);
				break;
			case SDLK_DOWN:
				do
				{
					if (++sel >= MenuOptions_MAX)
						sel = 0;
				}
				while (menu_items_disabled[sel]);
				
				JE_playSampleNum(S_CURSOR);
				break;
				
			case SDLK_LEFT:
				if (sel == MENU_SCALER)
				{
					do
					{
						if (temp_scaler == 0)
							temp_scaler = scalers_count;
						temp_scaler--;
					}
					while (!can_init_scaler(temp_scaler, fullscreen_enabled));
					
					JE_playSampleNum(S_CURSOR);
				}
				break;
			case SDLK_RIGHT:
				if (sel == MENU_SCALER)
				{
					do
					{
						temp_scaler++;
						if (temp_scaler == scalers_count)
							temp_scaler = 0;
					}
					while (!can_init_scaler(temp_scaler, fullscreen_enabled));
					
					JE_playSampleNum(S_CURSOR);
				}
				break;
				
			case SDLK_RETURN:
				switch (sel)
				{
				case MENU_ABOUT:
					JE_playSampleNum(S_SELECT);

					scroller_sine(about_text);

					memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
					JE_showVGA();
					fade_in = true;
					break;
					
				case MENU_FULLSCREEN:
					JE_playSampleNum(S_SELECT);

					if (!init_scaler(scaler, !fullscreen_enabled) && // try new fullscreen state
						!init_any_scaler(!fullscreen_enabled) &&     // try any scaler in new fullscreen state
						!init_scaler(scaler, fullscreen_enabled))    // revert on fail
					{
						exit(EXIT_FAILURE);
					}
					set_palette(colors, 0, 255); // for switching between 8 bpp scalers
					break;
					
				case MENU_SCALER:
					JE_playSampleNum(S_SELECT);

					if (scaler != temp_scaler)
					{
						if (!init_scaler(temp_scaler, fullscreen_enabled) &&   // try new scaler
							!init_scaler(temp_scaler, !fullscreen_enabled) &&  // try other fullscreen state
							!init_scaler(scaler, fullscreen_enabled))          // revert on fail
						{
							exit(EXIT_FAILURE);
						}
						set_palette(colors, 0, 255); // for switching between 8 bpp scalers
					}
					break;
					
				case MENU_JUKEBOX:
					JE_playSampleNum(S_SELECT);

					fade_black(10);
					jukebox();

					memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
					JE_showVGA();
					fade_in = true;
					break;
					
				case MENU_RETURN:
					quit = true;
					JE_playSampleNum(S_SPRING);
					break;
					
				case MenuOptions_MAX:
					assert(false);
					break;
				}
				break;
				
			case SDLK_ESCAPE:
				quit = true;
				JE_playSampleNum(S_SPRING);
				break;
				
			default:
				break;
			}
		}
	} while (!quit);
*/
}

int main( int argc, char *argv[] )
{
	mt_srand(time(NULL));

	// This should never happen, but until the game is loaded, be "in" a service menu
	inServiceMenu = true;

	printf("\nThis is... >> %s %s <<\n\n", opentyrian_str, opentyrian_version);

	printf("Copyright          (C) 2007-2020  The OpenTyrian Development Team\n");
	printf("Portions copyright (C) 2019-2020  Kaito Sinclaire\n\n");

	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions.  See the file GPL.txt for details.\n\n");

	if (SDL_Init(0))
	{
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	ArcTyr_loadConfig();

	JE_paramCheck(argc, argv);

	Episode_scan();

	init_video();

	JE_loadPals();
	JE_loadMainShapeTables("tyrian.shp");

	/* Default Options */
	youAreCheating = false;
	smoothScroll = true;

	if (!audio_disabled)
	{
		printf("initializing SDL audio...\n");

		init_audio();

		load_music();

		JE_loadSndFile("tyrian.snd", "voices.snd");
	}
	else
	{
		printf("audio disabled\n");
	}

	ARC_Identify();

	if (tyrian2000detected)
	{
		ARC_IdentifyPrint("");
		ARC_IdentifyWarn("Tyrian 2000 detected.");
		ARC_IdentifyWarn("Support for Tyrian 2000 is imperfect. Use with caution.");
	}

	I_KEY_init();
	I_JOY_init();

	if (record_demo)
	{
		ARC_IdentifyPrint("");
		ARC_IdentifyPrint("Demo recording active.");
	}

	JE_loadHelpText();
	/*debuginfo("Help text complete");*/

	PlayData_load();

	ARC_IdentifyPrint("");
	ARC_IdentifyPrint("All OK");

	if (goToWeaponCreator)
	{
#ifdef ENABLE_DEVTOOLS
		ARC_IdentifyPrint("Entering the weapon creator.");
		DEV_WeaponCreator();
#else
		fprintf(stderr, "ArcTyr was compiled without devtools.\n");
		JE_tyrianHalt(5);
#endif
	}

	inServiceMenu = false;
	if (setjmp(service_buffer) == 42069)
		ARC_Service();
	else
		ARC_IdentifyEnd();

	while (record_demo)
	{
#ifdef ENABLE_DEVTOOLS
		isInGame = false;
		DEV_RecordDemoInit();
		isInGame = true;
		JE_main();
		// Never returns
#else
		fprintf(stderr, "ArcTyr was compiled without devtools.\n");
		JE_tyrianHalt(5);
#endif
	}	

	for (; ; )
	{
		JE_initPlayerData();

		if (!Menu_titleScreen())
			ARC_NextIdleScreen();

		if (gameLoaded)
		{
			isInGame = true;
			JE_main();
			isInGame = false;
		}
	}

	JE_tyrianHalt(0);

	return 0;
}

__attribute__((noreturn)) void JE_tyrianHalt( JE_byte code )
{
	printf("Halting...\n");
	ArcTyr_saveConfig();

	deinit_audio();
	deinit_video();

	I_JOY_close();

	PlayData_free();

	free_main_shape_tables();

	free_sprite2s(&shapes6);

	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		if (digiFx[i])
			free(digiFx[i]);
	}

	free_pals();

	SDL_Quit();
	exit((code == 5) ? 0 : code);
}
