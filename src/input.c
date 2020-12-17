/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  input.c
/// \brief Revised input mechanisms

#include "arcade.h"
#include "opentyr.h"
#include "input.h"
#include "nortsong.h"
#include "player.h"
#include "video.h"

#include "video/scaler.h"

#include "arcade/service.h"

#include "SDL.h"

// -------------------------
// Global Button Assignments
// -------------------------

// These are the names of the assignments saved to the config file.
static const char *assignment_names[NUM_ASSIGNMENTS] = {
	"p1_up", "p1_down", "p1_left", "p1_right", "p1_fire", "p1_sidekick", "p1_mode",
	"p2_up", "p2_down", "p2_left", "p2_right", "p2_fire", "p2_sidekick", "p2_mode",
	"coin_slot_p1", "coin_slot_p2", "service_hot_debug", "coin_slot_service", "service_menu"
};

// True if the assignment allows for repeated keys when held.
// Currently this is only true of directional inputs.
static const bool assignment_can_repeat[NUM_ASSIGNMENTS] = {
	true, true, true, true, false, false, false,
	true, true, true, true, false, false, false,
	false, false, false, false, false
};

// The default assignment set, used for new installs / resets.
static const Assignment default_assignments[NUM_ASSIGNMENTS][4] = {
	{ {IT_KEY, SDLK_UP   }, {IT_JHAT, 0, 0, DIR_UP   } }, // INPUT_P1_UP
	{ {IT_KEY, SDLK_DOWN }, {IT_JHAT, 0, 0, DIR_DOWN } }, // INPUT_P1_DOWN,
	{ {IT_KEY, SDLK_LEFT }, {IT_JHAT, 0, 0, DIR_LEFT } }, // INPUT_P1_LEFT,
	{ {IT_KEY, SDLK_RIGHT}, {IT_JHAT, 0, 0, DIR_RIGHT} }, // INPUT_P1_RIGHT,

	{ {IT_KEY, SDLK_SPACE}, {IT_KEY, SDLK_KP_DIVIDE  }, {IT_JBTN, 0, 0} }, // INPUT_P1_FIRE,
	{ {IT_KEY, SDLK_z    }, {IT_KEY, SDLK_KP_MULTIPLY}, {IT_JBTN, 1, 0} }, // INPUT_P1_SKICK,
	{ {IT_KEY, SDLK_x    }, {IT_KEY, SDLK_KP_MINUS   }, {IT_JBTN, 2, 0} }, // INPUT_P1_MODE,

	{ {IT_KEY, SDLK_w}, {IT_JHAT, 0, 1, DIR_UP   } }, // INPUT_P2_UP,
	{ {IT_KEY, SDLK_s}, {IT_JHAT, 0, 1, DIR_DOWN } }, // INPUT_P2_DOWN,
	{ {IT_KEY, SDLK_a}, {IT_JHAT, 0, 1, DIR_LEFT } }, // INPUT_P2_LEFT,
	{ {IT_KEY, SDLK_d}, {IT_JHAT, 0, 1, DIR_RIGHT} }, // INPUT_P2_RIGHT,

	{ {IT_KEY, SDLK_r}, {IT_JBTN, 0, 1} }, // INPUT_P2_FIRE,
	{ {IT_KEY, SDLK_t}, {IT_JBTN, 1, 1} }, // INPUT_P2_SKICK,
	{ {IT_KEY, SDLK_y}, {IT_JBTN, 2, 1} }, // INPUT_P2_MODE,

	{ {IT_JBTN, 4, 0} }, // INPUT_P1_COIN,
	{ {IT_JBTN, 4, 1} }, // INPUT_P2_COIN,

	{ {IT_KEY, SDLK_ESCAPE } }, // INPUT_SERVICE_HOTDEBUG,   (SDLK_ESCAPE hardcoded)
	{ {IT_KEY, SDLK_F9     } }, // INPUT_SERVICE_COIN,       (SDLK_F9 hardcoded)
	{ {IT_KEY, SDLK_F10    } }, // INPUT_SERVICE_ENTER,      (SDLK_F10 hardcoded)
};

// The current set of button assignments.
Assignment button_assignments[NUM_ASSIGNMENTS][4];

JE_byte button_pressed[NUM_ASSIGNMENTS];
uint button_time_held[NUM_ASSIGNMENTS];

// Used for saving/displaying button assignments
static const char *hataxis_dirs[16] = {"Neg", "Up", "Rgt", "", "Dwn", "", "", "", "Lft", "", "", "", "", "", "", "Pos"};

// Used for loading config files
void I_readButtonCode( uint inputNum, uint subInput, const char *code )
{
	Assignment *a = &button_assignments[inputNum][subInput];
	memset(a, 0, sizeof(Assignment));

	switch (code[0])
	{
	case 'K':
		a->type = IT_KEY;
		a->value = atoi(code + 1);
		return;

	case 'J':
		a->jNum = atoi(code + 1);
		if (strlen(code) < 6) // sanity check
			return;
		a->value = atoi(code + 4);
		switch (code[3])
		{
		case 'B':
			a->type = IT_JBTN;
			return;

		case 'A':
		case 'H':
			// Yes, three nested switches. I hate everything.
			switch (code[6])
			{
				case 'U': a->dir = DIR_UP; break;
				case 'D': a->dir = DIR_DOWN; break;
				case 'L': a->dir = DIR_LEFT; break;
				case 'R': a->dir = DIR_RIGHT; break;
				case 'P': a->dir = DIR_POS; break;
				case 'N': a->dir = DIR_NEG; break;
				default: return; // invalid
			}
			a->type = (code[3] == 'A') ? IT_JAXIS : IT_JHAT;
			return;

		default: // invalid
			return;
		}
		return;

	default: // error / none
		return;
	}
}

// Used for saving config files
const char *I_getButtonCode( uint inputNum, uint subInput )
{
	static char bcbuf[16] = "";

	if (inputNum >= NUM_ASSIGNMENTS || subInput >= 4)
		return "";

	Assignment *a = &button_assignments[inputNum][subInput];
	switch (a->type)
	{
		case IT_KEY:
			snprintf(bcbuf, sizeof(bcbuf), "K%04u", a->value);
			break;
		case IT_JBTN:
			snprintf(bcbuf, sizeof(bcbuf), "J%02hhuB%02u", a->jNum, a->value);
			break;
		case IT_JHAT:
		case IT_JAXIS:
			snprintf(bcbuf, sizeof(bcbuf), "J%02hhu%c%02u%c", a->jNum, (a->type == IT_JHAT) ? 'H' : 'A', a->value, hataxis_dirs[a->dir & 0xF][0]);
			break;
		default:
			return "";
	}
	return bcbuf;
}

// Used for the service menu
const char *I_printableButtonCode( uint inputNum, uint subInput )
{
	static char pcbuf[32] = "";

	if (inputNum >= NUM_ASSIGNMENTS || subInput >= 4)
		return "";

	Assignment *a = &button_assignments[inputNum][subInput];
	switch (a->type)
	{
		case IT_KEY:
			snprintf(pcbuf, sizeof(pcbuf), "\"%s\"", SDL_GetKeyName(a->value));
			break;
		case IT_JBTN:
			snprintf(pcbuf, sizeof(pcbuf), "Joy%hhu Btn%u", a->jNum + 1, a->value + 1);
			break;
		case IT_JHAT:
		case IT_JAXIS:
			snprintf(pcbuf, sizeof(pcbuf), "Joy%hhu %s%u-%s", a->jNum + 1, (a->type == IT_JHAT) ? "Hat" : "Axis", a->value + 1, hataxis_dirs[a->dir & 0xF]);
			break;
		default:
			return "(not set)";
	}
	return pcbuf;
}

bool I_loadConfigAssignments( Config *config )
{
	ConfigSection *sec = config_find_section(config, "input", NULL);
	ConfigOption *opt;

	if (sec == NULL)
		return false;

	memset(button_assignments, 0, sizeof(button_assignments));

	for (size_t aNum = 0; aNum < NUM_ASSIGNMENTS; ++aNum)
	{
		opt = config_get_option(sec, assignment_names[aNum]);
		if (opt == NULL)
			continue;

		foreach_option_i_value(i, value, opt)
		{
			if (i >= 4)
				break;

			I_readButtonCode(aNum, i, value);
		}
	}

	// F9, F10, and ESCAPE are hardcoded
	button_assignments[INPUT_SERVICE_HOTDEBUG][0].type = IT_KEY;
	button_assignments[INPUT_SERVICE_HOTDEBUG][0].value = SDLK_ESCAPE;
	button_assignments[INPUT_SERVICE_COIN][0].type = IT_KEY;
	button_assignments[INPUT_SERVICE_COIN][0].value = SDLK_F9;
	button_assignments[INPUT_SERVICE_ENTER][0].type = IT_KEY;
	button_assignments[INPUT_SERVICE_ENTER][0].value = SDLK_F10;

	return true;
}

bool I_saveConfigAssignments( Config *config )
{
	ConfigSection *sec = config_find_or_add_section(config, "input", NULL);
	ConfigOption *opt;

	if (sec == NULL)
		return false;

	for (size_t aNum = 0; aNum < NUM_ASSIGNMENTS; ++aNum)
	{
		if (!(opt = config_set_option(sec, assignment_names[aNum], NULL))
		 || !(opt = config_set_value(opt, NULL)))
			return false;

		for (size_t i = 0; i < 4; ++i)
		{
			if (button_assignments[aNum][i].type == IT_NONE)
				break;

			if (!(opt = config_add_value(opt, I_getButtonCode(aNum, i))))
				return false;
		}
	}

	return true;
}

void I_resetConfigAssignments( void )
{
	memcpy(button_assignments, default_assignments, sizeof(button_assignments));
}


// ------------
// Input fuzzer
// ------------

bool inputFuzzing = false;

#ifdef ENABLE_DEVTOOLS
static int fuzzDelay = 0;

static void I_fuzzInputs( void )
{
	int i;
	if (--fuzzDelay < 0)
	{
		int rnd = mt_rand();
		memset(button_pressed, 0, sizeof(button_pressed));
		for (uint pnum = 0; pnum < COUNTOF(player); ++pnum)
		{
			i = pnum*7;
			switch (player[pnum].player_status)
			{
				case STATUS_NAMEENTRY:
					switch (rnd & 0x7)
					{
						case 0:  button_pressed[INPUT_P1_LEFT + i]  = true; break;
						default: button_pressed[INPUT_P1_RIGHT + i] = true; break;
					}
					button_pressed[INPUT_P1_DOWN + i] = (rnd & 0x700) ? false : true;
					button_pressed[INPUT_P1_FIRE + i] = (rnd & 0x3000) ? false : true;
					break;
				case STATUS_SELECT:
					// Input fuzzers auto start on "random", we just press fire whenever we feel like it
					button_pressed[INPUT_P1_FIRE + i] = (rnd & 0xF) ? false : true;
					break;
				default:
					switch (rnd & 0x7)
					{
						case 0: case 4: case 7:                                    break;
						case 1: case 2: button_pressed[INPUT_P1_DOWN + i]  = true; break;
						case 3:         button_pressed[INPUT_P1_UP + i]    = true; break;
						case 5:         button_pressed[INPUT_P1_LEFT + i]  = true; break;
						case 6:         button_pressed[INPUT_P1_RIGHT + i] = true; break;
					}
					button_pressed[INPUT_P1_FIRE + i] = (rnd & 0x18) ? true : false;
					button_pressed[INPUT_P1_SKICK + i] = (rnd & 0x20) ? true : false;
					button_pressed[INPUT_P1_MODE + i] = (rnd & 0x3FF0) ? false : true;
					break;
			}
			fuzzDelay += (rnd & 0xC000) >> 14;
			rnd >>= 16;
		}
	}

	for (i = INPUT_P1_UP; i <= INPUT_P2_MODE; ++i)
	{
		if (button_pressed[i])
			++button_time_held[i];
		else
			button_time_held[i] = 0;
	}
}
#endif


// -----------------
// Joystick handling
// -----------------

int num_joysticks = 0;
SDL_Joystick **joy_handles;

void I_JOY_init( void )
{
	ARC_IdentifyPrint("");
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
	{
		fprintf(stderr, "warning: failed to initialize joystick system: %s\n", SDL_GetError());
		return;
	}

	SDL_JoystickEventState(SDL_IGNORE);

	num_joysticks = SDL_NumJoysticks();
	if (num_joysticks > 0)
		joy_handles = malloc(num_joysticks * sizeof(SDL_Joystick*));
	if (!joy_handles)
		num_joysticks = 0;

	for (int j = 0; j < num_joysticks; j++)
	{
		joy_handles[j] = SDL_JoystickOpen(j);
		if (joy_handles[j] != NULL)
		{
			snprintf(tmpBuf.l, sizeof(tmpBuf.l), "Joystick detected: %s", SDL_JoystickName(j));
			ARC_IdentifyPrint(tmpBuf.l);
			snprintf(tmpBuf.l, sizeof(tmpBuf.l), "               (%d axes, %d buttons, %d hats)",
				SDL_JoystickNumAxes(joy_handles[j]), SDL_JoystickNumButtons(joy_handles[j]), SDL_JoystickNumHats(joy_handles[j]));
			ARC_IdentifyPrint(tmpBuf.l);
		}
	}

	if (num_joysticks == 0)
		ARC_IdentifyPrint("No joysticks were detected");
}

void I_JOY_close( void )
{
	if (joy_handles)
	{
		for (int i = 0; i < num_joysticks; ++i)
		{
			SDL_JoystickClose(joy_handles[i]);
		}
		free(joy_handles);
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


// -----------------
// Keyboard handling
// -----------------

Uint8 keys_active_this_time[SDLK_LAST] = {false}; // for extremely fast button presses
Uint8 keys_active[SDLK_LAST] = {false};

void I_KEY_init( void )
{
	if (inputFuzzing)
	{
#ifdef ENABLE_DEVTOOLS
		ARC_IdentifyPrint("");
		ARC_IdentifyPrint("Fuzzing enabled: Player inputs will be randomly made.");
#else
		fprintf(stderr, "ArcTyr was compiled without devtools.\n");
		JE_tyrianHalt(5);
#endif
	}

	// No builtin key repeat
	SDL_EnableKeyRepeat(0, 0);

	// Always show mouse cursor outside of fullscreen
	SDL_ShowCursor((fullscreen_enabled) ? SDL_DISABLE : SDL_ENABLE);
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

static void I_KEY_events( void )
{
	SDL_Event ev;

	memset(keys_active_this_time, 0, sizeof(keys_active_this_time));
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_KEYDOWN:
				if ((ev.key.keysym.mod & KMOD_CTRL) && ev.key.keysym.sym == SDLK_BACKSPACE)
				{
					printf("*** Emergency halt ***\n");
					JE_tyrianHalt(1);
				}

				if ((ev.key.keysym.mod & KMOD_ALT) && ev.key.keysym.sym == SDLK_RETURN)
				{
					printf("*** Fullscreen toggle ***\n");
					if (!init_scaler(scaler, !fullscreen_enabled) && // try new fullscreen state
					    !init_any_scaler(!fullscreen_enabled) &&     // try any scaler in new fullscreen state
					    !init_scaler(scaler, fullscreen_enabled))    // revert on fail
					{
						exit(EXIT_FAILURE);
					}
					SDL_ShowCursor((fullscreen_enabled) ? SDL_DISABLE : SDL_ENABLE);
					break;
				}
				keys_active[ev.key.keysym.sym] = 1;
				keys_active_this_time[ev.key.keysym.sym] = 1;
				break;
			case SDL_KEYUP:
				keys_active[ev.key.keysym.sym] = 0;
				break;

			case SDL_QUIT:
				// Halt requested by SDL
				JE_tyrianHalt(0);
				break;
		}
	}
}

// -
//
// -

void I_checkButtons( void )
{
	int i, j;
	JE_byte pressed;

	I_KEY_events();
	SDL_JoystickUpdate();

	if (inputFuzzing && !inServiceMenu)
	{
		I_fuzzInputs();
		i = INPUT_SERVICE_HOTDEBUG;
	}
	else
		i = 0;

	for (; i < NUM_ASSIGNMENTS; ++i)
	{
		pressed = 0;
		for (j = 0; j < 4; ++j)
		{
			Assignment *a = &button_assignments[i][j];
			switch (a->type)
			{
				case IT_KEY:
					if (keys_active[a->value] || keys_active_this_time[a->value])
					{
						pressed = true;
						goto end_assignment;
					}
					break;
				case IT_JBTN:
					if (a->jNum >= num_joysticks || !joy_handles[a->jNum])
						break;
					if (SDL_JoystickGetButton(joy_handles[a->jNum], a->value))
					{
						pressed = true;
						goto end_assignment;
					}
					break;
				case IT_JHAT:
					if (a->jNum >= num_joysticks || !joy_handles[a->jNum])
						break;
					if (SDL_JoystickGetHat(joy_handles[a->jNum], a->value) & a->dir)
					{
						pressed = true;
						goto end_assignment;
					}
					break;
				case IT_JAXIS:
					if (a->jNum >= num_joysticks || !joy_handles[a->jNum])
						break;
					if ((a->dir == DIR_POS && SDL_JoystickGetAxis(joy_handles[a->jNum], a->value) >=  AXIS_BOUNDARY)
					 || (a->dir == DIR_NEG && SDL_JoystickGetAxis(joy_handles[a->jNum], a->value) <= -AXIS_BOUNDARY))
					{
						pressed = true;
						goto end_assignment;
					}
					break;
				default:
					goto end_assignment;
			}
		}

		end_assignment:
		if ((button_pressed[i] = pressed))
			++button_time_held[i];
		else
			button_time_held[i] = 0;
	}

	// hardcoded responses
	// note: these are all separated so that if multiple coin ups happen
	// at the same time, they're all handled
	if (button_time_held[INPUT_P1_COIN] == 1)
		ARC_InsertCoin();
	if (button_time_held[INPUT_P2_COIN] == 1)
		ARC_InsertCoin();
	if (!inServiceMenu)
	{
		if (button_time_held[INPUT_SERVICE_COIN] == 1)
			ARC_InsertCoin();
		if (button_time_held[INPUT_SERVICE_ENTER] == 1)
			ARC_EnterService(); // DOES NOT RETURN
	}
}

void I_assignInput(Player *pl, uint pNum)
{
	memcpy(pl->last_buttons, pl->buttons, sizeof(pl->buttons));
	memcpy(pl->buttons, &button_pressed[(pNum == 2) ? INPUT_P2_UP : INPUT_P1_UP], NUM_BUTTONS);
}

bool I_inputMade(uint i)
{
	if (button_time_held[i] == 1)
		return true;
	return (assignment_can_repeat[i] && button_time_held[i] > 15 && !(button_time_held[i] % 3));
}

bool I_inputForMenu(uint *i, uint stop)
{
	for (; *i <= stop; ++*i)
	{
		if (button_time_held[*i] == 1)
			return true;
		if (assignment_can_repeat[*i] && button_time_held[*i] > 15 && !(button_time_held[*i] % 3))
			return true;
	}
	return false;
}

bool I_waitOnInputForMenu(uint start, uint stop, uint wait)
{
	uint button;

	while (!wait || --wait)
	{
		// Wait a frame
		JE_showVGA();
		wait_delay();
		setjasondelay(2);

		I_checkButtons();

		button = start;
		if (I_inputForMenu(&button, stop))
			return true;
	}

	return false;
}

static JE_byte _code_input_count[2] = {0, 0};
static JE_byte _code_input_progress[2][16];

void I_initCodeInput(uint pNum)
{
	pNum -= 1;
	_code_input_count[pNum] = 0;
	memset(_code_input_progress[pNum], 0xFF, sizeof(JE_byte) * 16);
}

// returns NULL if no code / in progress
// returns pointer to code array if code input just ended
JE_byte* I_checkForCodeInput(uint pNum, JE_byte* out_len)
{
	static JE_byte code_out_buf[16];
	*out_len = 0;

	pNum -= 1;
	// Check for codes if special mode button is held
	if (button_time_held[INPUT_P1_MODE + (7 * pNum)] >= 1)
	{
		uint i, target = INPUT_P1_SKICK + (7 * pNum);
		if (_code_input_count[pNum] < 16)
		{
			JE_byte to_append = 1;

			for (i = INPUT_P1_UP + (7 * pNum); i <= target; ++i)
			{ // Find inputs just made and add to code progress
				if (button_time_held[i] == 1)
				{
					_code_input_progress[pNum][_code_input_count[pNum]++] = to_append;
					break; // Only append one input per frame
				}
				to_append <<= 1;
			}
		}
		for (i = INPUT_P1_UP + (7 * pNum); i <= target; ++i)
		{ // Suppress all inputs, too
			if (button_time_held[i] >= 1)
				button_time_held[i] = 2;
		}
		return NULL;
	}

	// No input is being made (or a zero input code was made)
	if (_code_input_count[pNum] == 0)
		return NULL;

	// Just released, return code
	memset(code_out_buf, 0xFF, sizeof(JE_byte) * 16);
	memcpy(code_out_buf, _code_input_progress[pNum], sizeof(JE_byte) * _code_input_count[pNum]);
	*out_len = _code_input_count[pNum];
	_code_input_count[pNum] = 0;
	return code_out_buf;
}

bool hasRequestedToSkip = false;

void I_checkStatus( void )
{
	++arcTextTimer;
	arcTextTimer %= 200;

	I_checkButtons();

	ARC_HandlePlayerStatus(&player[0], 1);
	ARC_HandlePlayerStatus(&player[1], 2);
}

bool I_checkSkipAndStatus( void )
{
	++arcTextTimer;
	arcTextTimer %= 200;

	I_checkButtons();

	ARC_HandlePlayerStatus(&player[0], 1);
	ARC_HandlePlayerStatus(&player[1], 2);

	return ((player[0].player_status == STATUS_INGAME && button_time_held[INPUT_P1_FIRE] == 1)
		||  (player[1].player_status == STATUS_INGAME && button_time_held[INPUT_P2_FIRE] == 1));
}

bool I_checkSkipScene( void )
{
	++arcTextTimer;
	arcTextTimer %= 200;

	// if an ingame player presses the fire button
	I_checkButtons();
	return ((player[0].player_status == STATUS_INGAME && button_time_held[INPUT_P1_FIRE] == 1)
		||  (player[1].player_status == STATUS_INGAME && button_time_held[INPUT_P2_FIRE] == 1));
}

bool I_checkSkipSceneFromAnyone( void )
{
	++arcTextTimer;
	arcTextTimer %= 200;

	// ... what this actually means: a fire button
	I_checkButtons();
	return ((button_time_held[INPUT_P1_FIRE] == 1) || (button_time_held[INPUT_P2_FIRE] == 1));
}

// ----------------
// Remapping inputs
// ----------------

static bool I_RemapInput( uint inputNum, uint subInput, Assignment *map )
{
	if (map->type == IT_NONE)
	{
		if (subInput < 3)
			memmove(&button_assignments[inputNum][subInput], &button_assignments[inputNum][subInput+1], sizeof(Assignment)*(3-subInput));
		button_assignments[inputNum][3].type = IT_NONE;
		return true;
	}
	if (map->type == IT_KEY && ((map->value >= SDLK_F9 && map->value <= SDLK_F10) || map->value == SDLK_ESCAPE))
		return false;

	button_assignments[inputNum][subInput].type = map->type;
	button_assignments[inputNum][subInput].value = map->value;
	button_assignments[inputNum][subInput].jNum = map->jNum;
	button_assignments[inputNum][subInput].dir = map->dir;
	button_time_held[inputNum] = 1;
	return true;
}

bool I_PromptToRemapButton( uint inputNum, uint subInput )
{
	SDL_Event ev;
	static Assignment newMap = {IT_NONE};

	// Make sure all events up to this point are handled.
	I_KEY_events();
	SDL_JoystickUpdate();

	// Temporarily allow joystick events.
	SDL_JoystickEventState(SDL_ENABLE);

	newMap.type = IT_NONE;
	newMap.value = newMap.dir = newMap.jNum = 0;
	while (true)
	{
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
				case SDL_QUIT:
					// Halt requested by SDL
					JE_tyrianHalt(0);
					break;
				case SDL_KEYUP:
					// Still need this to make sure the button that was pressed to get into
					// input assignment mode is correctly considered released
					keys_active[ev.key.keysym.sym] = 0;
					break;

				case SDL_KEYDOWN:
					newMap.type = IT_KEY;
					newMap.value = ev.key.keysym.sym;
					goto exitloop;
				case SDL_JOYBUTTONDOWN:
					newMap.type = IT_JBTN;
					newMap.jNum = ev.jbutton.which;
					newMap.value = ev.jbutton.button;
					goto exitloop;
				case SDL_JOYHATMOTION:
					// Only assign if we get exactly one direction
					if (ev.jhat.value == SDL_HAT_UP)
						newMap.dir = DIR_UP;
					else if (ev.jhat.value == SDL_HAT_RIGHT)
						newMap.dir = DIR_RIGHT;
					else if (ev.jhat.value == SDL_HAT_DOWN)
						newMap.dir = DIR_DOWN;
					else if (ev.jhat.value == SDL_HAT_LEFT)
						newMap.dir = DIR_LEFT;
					else break;
					newMap.type = IT_JHAT;
					newMap.jNum = ev.jhat.which;
					newMap.value = ev.jhat.hat;
					goto exitloop;
				case SDL_JOYAXISMOTION:
					if (ev.jaxis.value >= AXIS_BOUNDARY)
						newMap.dir = DIR_POS;
					else if (ev.jaxis.value <= -AXIS_BOUNDARY)
						newMap.dir = DIR_NEG;
					else break;
					newMap.type = IT_JAXIS;
					newMap.jNum = ev.jaxis.which;
					newMap.value = ev.jaxis.axis;
					goto exitloop;
				default:
					break;
			}
		}

		JE_showVGA();
		wait_delay();
		setjasondelay(2);
	}

	exitloop:
	// Disallow joystick events upon exiting.
	SDL_JoystickEventState(SDL_IGNORE);

	if (newMap.type == button_assignments[inputNum][subInput].type && newMap.value == button_assignments[inputNum][subInput].value
	 && newMap.jNum == button_assignments[inputNum][subInput].jNum && newMap.dir == button_assignments[inputNum][subInput].dir)
	{
		newMap.type = IT_NONE;
	}
	return I_RemapInput(inputNum, subInput, &newMap);
}


// -----------
// Demo inputs
// -----------

bool play_demo = false, record_demo = false;
Uint8 demo_num = 0;
FILE *demo_file = NULL;
Uint8 demo_keys[2];
Uint64 demo_seed = 0;
Uint64 demo_record_time = 0;
JE_byte demoDifficulty = 0;

bool I_readDemoKeys( void )
{
	demo_keys[0] = fgetc(demo_file);
	demo_keys[1] = fgetc(demo_file);
	return !feof(demo_file);
}

void I_demoKeysToInput(Player *pl, uint pNum)
{
	--pNum;
	memcpy(pl->last_buttons, pl->buttons, sizeof(pl->buttons));
	pl->buttons[BUTTON_UP]    = (bool)(demo_keys[pNum] & (1 << 0));
	pl->buttons[BUTTON_DOWN]  = (bool)(demo_keys[pNum] & (1 << 1));
	pl->buttons[BUTTON_LEFT]  = (bool)(demo_keys[pNum] & (1 << 2));
	pl->buttons[BUTTON_RIGHT] = (bool)(demo_keys[pNum] & (1 << 3));
	pl->buttons[BUTTON_FIRE]  = (bool)(demo_keys[pNum] & (1 << 4));
	pl->buttons[BUTTON_SKICK] = (bool)(demo_keys[pNum] & (1 << 5));
	pl->buttons[BUTTON_MODE]  = (bool)(demo_keys[pNum] & (1 << 6));
}
