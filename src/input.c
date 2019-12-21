/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
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
	"coin_slot_p1", "coin_slot_p2", "coin_slot_service", "service_menu"
};

// True if the assignment allows for repeated keys when held.
// Currently this is only true of directional inputs.
static const bool assignment_can_repeat[NUM_ASSIGNMENTS] = {
	true, true, true, true, false, false, false,
	true, true, true, true, false, false, false,
	false, false, false, false
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

	{ {IT_KEY, SDLK_F7}, {IT_JBTN, 4, 0} }, // INPUT_P1_COIN, (SDLK_F7 hardcoded)
	{ {IT_KEY, SDLK_F8}, {IT_JBTN, 4, 1} }, // INPUT_P2_COIN, (SDLK_F8 hardcoded)

	{ {IT_KEY, SDLK_F9 } }, // INPUT_SERVICE_COIN,  (SDLK_F9 hardcoded)
	{ {IT_KEY, SDLK_F10} }, // INPUT_SERVICE_ENTER, (SDLK_F10 hardcoded)
};

// The current set of button assignments.
Assignment button_assignments[NUM_ASSIGNMENTS][4];

JE_byte button_pressed[NUM_ASSIGNMENTS];
uint button_time_held[NUM_ASSIGNMENTS];

// Used for saving/displaying button assignments
static const char *hataxis_dirs[16] = {"", "Up", "Rgt", "", "Dwn", "", "", "", "Lft", "", "", "", "", "", "", ""};

// Used for loading config files
void I_readButtonCode( uint assignment, uint i, const char *code )
{
	Assignment *a = &button_assignments[assignment][i];
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
const char *I_getButtonCode( uint assignment, uint i )
{
	static char bcbuf[16] = "";

	if (assignment >= NUM_ASSIGNMENTS || i >= 4)
		return "";

	Assignment *a = &button_assignments[assignment][i];
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
const char *I_printableButtonCode( uint assignment, uint i )
{
	static char pcbuf[32] = "";

	if (assignment >= NUM_ASSIGNMENTS || i >= 4)
		return "";

	Assignment *a = &button_assignments[assignment][i];
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

	// F7-F10 are hardcoded
	button_assignments[INPUT_P1_COIN][0].type = button_assignments[INPUT_P2_COIN][0].type = IT_KEY;
	button_assignments[INPUT_P1_COIN][0].value = SDLK_F7;
	button_assignments[INPUT_P2_COIN][0].value = SDLK_F8;
	button_assignments[INPUT_SERVICE_COIN][0].type = button_assignments[INPUT_SERVICE_ENTER][0].type = IT_KEY;
	button_assignments[INPUT_SERVICE_COIN][0].value = SDLK_F9;
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
	joy_handles = malloc(num_joysticks * sizeof(SDL_Joystick*));

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

// Debugging stuff -- checks for even normally unmapped keys
bool I_KEY_pressed( uint code )
{
	return keys_active_this_time[code];
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

	for (i = 0; i < NUM_ASSIGNMENTS; ++i)
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

bool I_waitOnInputForMenu( uint start, uint stop, uint wait )
{
	uint button;

	while (!wait || --wait)
	{
		I_checkButtons();

		button = start;
		if (I_inputForMenu(&button, stop))
			return true;

		// Wait a frame
		JE_showVGA();
		wait_delay();
		setjasondelay(2);
	}

	return false;
}

bool I_anyButton( void )
{
	// ... what this actually means: a fire button
	I_checkButtons();
	return ((button_time_held[INPUT_P1_FIRE] == 1) || (button_time_held[INPUT_P2_FIRE] == 1));
}

// ----------------
// Remapping inputs
// ----------------

static bool I_RemapInput( uint assignment, uint i, JE_byte type, uint value )
{
	if (type == IT_NONE)
	{
		if (i < 3)
			memmove(&button_assignments[assignment][i], &button_assignments[assignment][i+1], sizeof(Assignment)*(3-i));
		button_assignments[assignment][3].type = IT_NONE;
		return true;
	}
	if (type == IT_KEY && value >= SDLK_F7 && value <= SDLK_F10)
		return false;

	button_assignments[assignment][i].type = type;
	button_assignments[assignment][i].value = value;
	button_time_held[i] = 1;
	return true;
}

bool I_PromptToRemapButton( uint assignment, uint input )
{
	JE_byte type = IT_NONE;
	uint value = 0;

	int i;

	while (true)
	{
		I_KEY_events();
		SDL_JoystickUpdate();

		// Check all keyboard keys.
		for (i = 0; i < SDLK_LAST && !keys_active_this_time[i]; ++i);
		if (i < SDLK_LAST)
		{
			type = IT_KEY;
			value = i;
			break;
		}

		JE_showVGA();
		wait_delay();
		setjasondelay(2);
	}
	if (type == button_assignments[assignment][input].type && value == button_assignments[assignment][input].value)
		return I_RemapInput(assignment, input, IT_NONE, 0);
	return I_RemapInput(assignment, input, type, value);
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
	pl->buttons[BUTTON_MODE]  = (bool)(demo_keys[pNum] & (1 << 5));
	pl->buttons[BUTTON_SKICK] = (bool)(demo_keys[pNum] & (1 << 6));
}
