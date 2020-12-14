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
#include "version.h"
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

const char *program_name = "ArcTyr";
const char *program_date = __DATE__;
const char *program_time = __TIME__;

const char *program_version_short = GIT_REV_SHORT;
const char *program_version_full = GIT_REV_FULL;

TempStringBuffer tmpBuf = {"", ""};

JE_byte isInGame = false;

// T2000
bool tyrian2000detected = false;

int main( int argc, char *argv[] )
{
	mt_srand(time(NULL));

	// This should never happen, but until the game is loaded, be "in" a service menu
	inServiceMenu = true;

	printf("\nThis is... >> %s %s <<\n\n", program_name, program_version_full);

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

	if (!ArcTyr_loadConfig())
		isFirstRun = true;

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
