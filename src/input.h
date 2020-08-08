/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  input.h
/// \brief Revised input mechanisms

#include "opentyr.h"
#include "player.h"

#include "lib/config_file.h"

#include "SDL.h"

#define SDL_POLL_INTERVAL 5

// enum for case-styled menu input
enum {
	INPUT_P1_UP = 0,
	INPUT_P1_DOWN,
	INPUT_P1_LEFT,
	INPUT_P1_RIGHT,
	INPUT_P1_FIRE,
	INPUT_P1_SKICK,
	INPUT_P1_MODE,
	INPUT_P2_UP,
	INPUT_P2_DOWN,
	INPUT_P2_LEFT,
	INPUT_P2_RIGHT,
	INPUT_P2_FIRE,
	INPUT_P2_SKICK,
	INPUT_P2_MODE,

	INPUT_P1_COIN,
	INPUT_P2_COIN,

	INPUT_SERVICE_HOTDEBUG,
	INPUT_SERVICE_COIN,
	INPUT_SERVICE_ENTER,

	NUM_ASSIGNMENTS,
};

enum {
	IT_NONE = 0,
	IT_KEY,
	IT_JBTN,
	IT_JHAT,
	IT_JAXIS,
};

enum {
	DIR_UP    = SDL_HAT_UP,
	DIR_DOWN  = SDL_HAT_DOWN,
	DIR_LEFT  = SDL_HAT_LEFT,
	DIR_RIGHT = SDL_HAT_RIGHT,
	DIR_NEG   = 0,
	DIR_POS   = 15,
};

// At what point we consider an axis "pressed" or "held"
#define AXIS_BOUNDARY 24576

typedef struct {
	JE_byte type; // see IT_ above
	uint value; // keysym, or button hat or axis number
	JE_byte jNum; // joystick number, for 
	JE_byte dir; // direction for hat or axis
} Assignment;

Assignment button_assignments[NUM_ASSIGNMENTS][4];

// Development tool
bool inputFuzzing;

void I_JOY_init( void );
void I_JOY_close( void );

void I_KEY_init( void );

void I_checkButtons( void );
void I_assignInput( Player *pl, uint pNum );
bool I_inputMade(uint i);
bool I_inputForMenu( uint *i, uint stop );
bool I_waitOnInputForMenu( uint start, uint stop, uint wait );

extern bool hasRequestedToSkip;
void I_checkStatus( void );
bool I_checkSkipAndStatus( void );
bool I_checkSkipScene( void );
bool I_checkSkipSceneFromAnyone( void );

void I_readButtonCode( uint inputNum, uint subInput, const char *code );
const char *I_getButtonCode( uint inputNum, uint subInput );
const char *I_printableButtonCode( uint inputNum, uint subInput );

bool I_loadConfigAssignments( Config *config );
bool I_saveConfigAssignments( Config *config );
void I_resetConfigAssignments( void );

bool I_PromptToRemapButton( uint inputNum, uint subInput );

// Demo recording / playback
extern bool play_demo, record_demo;
extern Uint8 demo_num;
extern FILE *demo_file;
extern Uint8 demo_keys[2];
extern Uint64 demo_seed;
extern Uint64 demo_record_time;
extern JE_byte demoDifficulty;

bool I_readDemoKeys( void );
void I_demoKeysToInput( Player *pl, uint pNum );
