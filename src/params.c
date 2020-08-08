/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  params.c
/// \brief Parameters on load.

#include "file.h"
#include "input.h"
#include "loudness.h"
#include "opentyr.h"
#include "params.h"

#include "lib/arg_parse.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

JE_boolean goToWeaponCreator = false;
JE_byte shutdownCode = 0;

void JE_paramCheck( int argc, char *argv[] )
{
	const Options options[] =
	{
		{ 'h', 'h', "help",              false },
		
		{ 's', 's', "no-sound",          false },
		{ 't', 't', "data",              true },
		
		{ 'n', 'n', "net",               true },
		{ 'N', 'N', "net-server",        true },

		{ 'z', 'z', "shutdown",          true },

		{ 'e', 'e', "shot-edit",         false },
		{ 'r', 'r', "record",            false },
		{ 'f', 'f', "fuzz",              false },
		
		{ 0, 0, NULL, false}
	};
	
	Option option;
	
	for (; ; )
	{
		option = parse_args(argc, (const char **)argv, options);
		
		if (option.value == NOT_OPTION)
			break;
		
		switch (option.value)
		{
		case INVALID_OPTION:
		case AMBIGUOUS_OPTION:
		case OPTION_MISSING_ARG:
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
			
		case 'h':
			printf("Usage: %s [OPTION...]\n\n"
			       "Options:\n"
			       "  -h, --help                   Show help about options\n\n"
			       "  -s, --no-sound               Disable audio\n"
			       "  -t, --data=DIR               Set Tyrian data directory\n\n"
			       "  -z, --shutdown               Send special exit code on exit\n\n"
#ifdef ENABLE_DEVTOOLS
			       "  -e, --shot-edit              Open interactive shot viewer/editor\n"
			       "  -r, --record                 Record demos\n"
			       "  -f, --fuzz                   Randomly fuzz player inputs for testing\n"
#endif
			       "", argv[0]);
			exit(0);
			break;
			
		case 's':
			// Disables sound/music usage
			audio_disabled = true;
			break;
			
		// set custom Tyrian data directory
		case 't':
			custom_data_dir = option.arg;
			break;
			
		case 'n':
		case 'N':
			fprintf(stderr, "%s: error: netplay currently unsupported\n", argv[0]);
			exit(EXIT_FAILURE);
			break;

		case 'r':
			record_demo = true;
			break;

		case 'z':
		{
			int temp = atoi(option.arg);
			if (temp > 10 && temp <= 128)
				shutdownCode = temp;
			else
			{
				fprintf(stderr, "%s: error: invalid shutdown exit code\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		}
		case 'e':
		{
			goToWeaponCreator = true;
			break;
		}
		case 'f':
		{
			inputFuzzing = true;
			break;
		}
		default:
			assert(false);
			break;
		}
	}
}

