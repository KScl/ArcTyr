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
#include "file.h"
#include "input.h"
#include "loudness.h"
#include "opentyr.h"
#include "params.h"
#include "xmas.h"

#include "lib/arg_parse.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

JE_boolean goToWeaponCreator = false;
JE_byte shutdownCode = 0;

/* YKS: Note: LOOT cheat had non letters removed. */
const char pars[][9] = {
	"LOOT", "RECORD", "NOJOY", "CONSTANT", "DEATH", "NOSOUND", "NOXMAS", "YESXMAS"
};

void JE_paramCheck( int argc, char *argv[] )
{
	const Options options[] =
	{
		{ 'h', 'h', "help",              false },
		
		{ 's', 's', "no-sound",          false },
		{ 'x', 'x', "no-xmas",           false },
		
		{ 't', 't', "data",              true },
		
		{ 'n', 'n', "net",               true },
		{ 'N', 'N', "net-server",        true },
		
		{ 'X', 'X', "xmas",              false },
		{ 'c', 'c', "constant",          false },
		{ 'r', 'r', "record",            false },
		{ 'l', 'l', "loot",              false },

		{ 'z', 'z', "shutdown",          true },

		{ 258, 0, "weapon-creator",      false },
		
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
			       "  -x, --no-xmas                Disable Christmas mode\n\n"
			       "  -t, --data=DIR               Set Tyrian data directory\n\n"
			       "  -n, --net=HOST[:PORT]        Start a networked game\n"
			       "  --net-player-name=NAME       Sets local player name in a networked game\n"
			       "  --net-player-number=NUMBER   Sets local player number in a networked game\n"
			       "                               (1 or 2)\n"
			       "  -p, --net-port=PORT          Local port to bind (default is 1333)\n"
			       "  -d, --net-delay=FRAMES       Set lag-compensation delay (default is 1)\n", argv[0]);
			exit(0);
			break;
			
		case 's':
			// Disables sound/music usage
			audio_disabled = true;
			break;
			
		case 'x':
			xmas = false;
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
			
		case 'X':
			xmas = true;
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
		case 258:
		{
			goToWeaponCreator = true;
			break;
		}
		default:
			assert(false);
			break;
		}
	}
	
	// legacy parameter support
	for (int i = option.argn; i < argc; ++i)
	{
		for (uint j = 0; j < strlen(argv[i]); ++j)
			argv[i][j] = toupper((unsigned char)argv[i][j]);
		
		for (uint j = 0; j < COUNTOF(pars); ++j)
		{
			if (strcmp(argv[i], pars[j]) == 0)
			{
				switch (j)
				{
				case 0:
					break;
				case 1:
					record_demo = true;
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					break;
				case 5:
					audio_disabled = true;
					break;
				case 6:
					xmas = false;
					break;
				case 7:
					xmas = true;
					break;
					
				default:
					assert(false);
					break;
				}
			}
		}
	}
}

