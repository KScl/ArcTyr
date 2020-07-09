/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  helptext.c
/// \brief Functions to read pascal strings, etc

#include "config.h"
#include "episodes.h"
#include "file.h"
#include "helptext.h"
#include "menus.h"
#include "opentyr.h"

#include <assert.h>
#include <string.h>

char pName[21][16];                                                      /* [1..21] of string [15] */
char miscText[HELPTEXT_MISCTEXT_COUNT][42];                              /* [1..68] of string [41] */
char mainMenuHelp[34][66];
char outputs[9][31];                                                     /* [1..9] of string [30] */
char inGameText[6][21];                                                  /* [1..6] of string [20] */
char detailLevel[6][13];                                                 /* [1..6] of string [12] */
char gameSpeedText[6][13];                                               /* [1..5] of string [12] */
char difficultyNameB[11][21];                                            /* [0..9] of string [20] */

void decrypt_pascal_string( char *s, int len )
{
	static const unsigned char crypt_key[] = { 204, 129, 63, 255, 71, 19, 25, 62, 1, 99 };

	if (len > 0)
	{
		for (int i = len - 1; i > 0; --i)
			s[i] ^= crypt_key[i % sizeof(crypt_key)] ^ s[i - 1];
		s[0] ^= crypt_key[0];		
	}
}

void read_encrypted_pascal_string( char *s, int size, FILE *f )
{
	int len = getc(f);
	if (len != EOF)
	{
		int skip = MAX((len + 1) - size, 0);
		assert(skip == 0);

		len -= skip;
		efread(s, 1, len, f);
		if (size > 0)
			s[len] = '\0';
		fseek(f, skip, SEEK_CUR);

		decrypt_pascal_string(s, len);
	}
}

void skip_pascal_string( FILE *f )
{
	int len = getc(f);
	fseek(f, len, SEEK_CUR);
}

void skip_strings_until_blank( FILE *f )
{
	int len;
	while ((len = getc(f)))
		fseek(f, len, SEEK_CUR);
}

void JE_loadHelpText( void )
{	
	FILE *f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
	efread(&episode1DataLoc, sizeof(JE_longint), 1, f);

	skip_strings_until_blank(f); // Skipped: Online Help

	// Planet names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(pName); ++i)
		read_encrypted_pascal_string(pName[i], sizeof(pName[i]), f);
	skip_strings_until_blank(f);

	// Miscellaneous text
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(miscText); ++i)
		read_encrypted_pascal_string(miscText[i], sizeof(miscText[i]), f);
	skip_strings_until_blank(f);

	skip_strings_until_blank(f); // Skipped: Small Misc Text
	skip_strings_until_blank(f); // Skipped: Key Names
	skip_strings_until_blank(f); // Skipped: Main Menu

	// Event text
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(outputs); ++i)
		read_encrypted_pascal_string(outputs[i], sizeof(outputs[i]), f);
	skip_strings_until_blank(f);

	skip_strings_until_blank(f); // Skipped: Help Topics

	/*Main Menu Help*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(mainMenuHelp); ++i)
		read_encrypted_pascal_string(mainMenuHelp[i], sizeof(mainMenuHelp[i]), f);
	skip_strings_until_blank(f);

	skip_strings_until_blank(f); // Skipped: Menu 1 - Main
	skip_strings_until_blank(f); // Skipped: Menu 2 - Items
	skip_strings_until_blank(f); // Skipped: Menu 3 - Options

	// InGame Menu
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(inGameText); ++i)
		read_encrypted_pascal_string(inGameText[i], sizeof(inGameText[i]), f);
	skip_strings_until_blank(f);

	// Detail Level
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(detailLevel); ++i)
		read_encrypted_pascal_string(detailLevel[i], sizeof(detailLevel[i]), f);
	skip_strings_until_blank(f);

	// Game speed text
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(gameSpeedText) - 1; ++i)
		read_encrypted_pascal_string(gameSpeedText[i], sizeof(gameSpeedText[i]), f);
	strncpy(gameSpeedText[5], "Unbounded", sizeof(gameSpeedText[5]));
	skip_strings_until_blank(f);

	// Episode names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(episode_name); ++i)
		read_encrypted_pascal_string(episode_name[i], sizeof(episode_name[i]), f);
	skip_strings_until_blank(f);

	skip_strings_until_blank(f); // Skipped: Difficulty names A (we use the other table)
	skip_strings_until_blank(f); // Skipped: Gameplay mode names
	skip_strings_until_blank(f); // Skipped: Menu 10 - 2Player Main
	skip_strings_until_blank(f); // Skipped: Input Devices
	skip_strings_until_blank(f); // Skipped: Network text
	skip_strings_until_blank(f); // Skipped: Menu 11 - 2Player Network

	// HighScore Difficulty Names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(difficultyNameB); ++i)
		read_encrypted_pascal_string(difficultyNameB[i], sizeof(difficultyNameB[i]), f);

	// No reason to dig any deeper into the file
	// Skipped: Menu 12 - Network Options
	// Skipped: Menu 13 - Joystick
	// Skipped: Joystick Button Assignments
	// Skipped: SuperShips - For Super Arcade Mode
	// Skipped: SuperShips Input Commands
	// Skipped: Secret DESTRUCT game
	// Skipped: Secret DESTRUCT weapons
	// Skipped: Secret DESTRUCT modes
	// Skipped: Ship Info (interleaved)

	fclose(f);
}

