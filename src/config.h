/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  config.h
/// \brief Managing configuration files

#ifndef CONFIG_H
#define CONFIG_H

#include "opentyr.h"

#include "lib/config_file.h"

#include <stdio.h>
#include "SDL.h"


#define SAVE_FILES_NUM (11 * 2)

/* These are necessary because the size of the structure has changed from the original, but we
   need to know the original sizes in order to find things in TYRIAN.SAV */
#define SAVE_FILES_SIZE 2398
#define SIZEOF_SAVEGAMETEMP SAVE_FILES_SIZE + 4 + 100
#define SAVE_FILE_SIZE (SIZEOF_SAVEGAMETEMP - 4)

/*#define SAVE_FILES_SIZE (2502 - 4)
#define SAVE_FILE_SIZE (SAVE_FILES_SIZE)*/

typedef SDLKey JE_KeySettingType[8]; /* [1..8] */
typedef JE_byte JE_PItemsType[12]; /* [1..12] */

typedef JE_byte JE_EditorItemAvailType[100]; /* [1..100] */

extern JE_boolean smoothies[9];
extern JE_byte starShowVGASpecialCode;
extern JE_shortint difficultyLevel, oldDifficultyLevel, initialDifficulty;

extern char levelName[11];
extern JE_byte mainLevel, nextLevel;
extern JE_shortint levelFilter, levelFilterNew, levelBrightness, levelBrightnessChg;
extern JE_boolean filtrationAvail, filterActive, filterFade, filterFadeStart;
extern JE_boolean twoPlayerLinked;
extern JE_byte SAPowerupBag[5];
extern JE_byte superArcadePowerUp;
extern JE_real linkGunDirec;
extern JE_byte secretHint;
extern JE_byte background3over;
extern JE_byte background2over;
extern JE_byte gammaCorrection;
extern JE_boolean explosionTransparent, youAreCheating, background2, smoothScroll, wild, superWild, starActive, topEnemyOver, skyEnemyOverAll, background2notTransparent;
extern JE_byte fastPlay;
extern JE_boolean pentiumMode;
extern JE_byte gameSpeed;
extern JE_byte processorType;

extern Config opentyrian_config;

bool ArcTyr_loadConfig( void );
bool ArcTyr_saveConfig( void );
void JE_initProcessorType( void );
void JE_setNewGameSpeed( void );
const char *get_user_directory( void );

#endif /* CONFIG_H */

