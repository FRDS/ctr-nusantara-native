#include <platform/native_input.h>

#include <macros.h>
#include "psx/libpad.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_INPUT_MAX_CONTROLLERS  2
#define NATIVE_INPUT_PAD_DIGITAL      0x41
#define NATIVE_INPUT_PAD_ANALOG       0x73
#define NATIVE_INPUT_PAD_DISCONNECT   0xff
#define NATIVE_INPUT_AXIS_DEADZONE    500
#define NATIVE_INPUT_MAP_FLAG_AXIS    0x4000
#define NATIVE_INPUT_MAP_FLAG_INVERSE 0x8000

// NOTE(aalhendi): Native input preserves behavior from PsyCross's
// MIT-licensed pad implementation while moving host ownership into ctr-native.
// See THIRD_PARTY_NOTICES.md.

struct NativeInputKeyboardMapping
{
	s32 id;

	s32 kc_square, kc_circle, kc_triangle, kc_cross;

	s32 kc_l1, kc_l2, kc_l3;
	s32 kc_r1, kc_r2, kc_r3;

	s32 kc_start, kc_select;

	s32 kc_dpad_left, kc_dpad_right, kc_dpad_up, kc_dpad_down;
};

struct NativeInputControllerMapping
{
	s32 id;

	s32 gc_square, gc_circle, gc_triangle, gc_cross;

	s32 gc_l1, gc_l2, gc_l3;
	s32 gc_r1, gc_r2, gc_r3;

	s32 gc_start, gc_select;

	s32 gc_dpad_left, gc_dpad_right, gc_dpad_up, gc_dpad_down;

	s32 gc_axis_left_x, gc_axis_left_y;
	s32 gc_axis_right_x, gc_axis_right_y;
};

struct NativeInputPadSnapshot
{
	u8 status;
	u8 id;
	u16 buttons;
	u8 analog[4];
	s32 connected;
};

struct NativeInputController
{
	Sint32 deviceIndex;
	SDL_JoystickID instanceId;
	SDL_GameController *controller;
	u8 *padData;
	s32 analogEnabled;
	s32 switchingAnalog;
	struct NativeInputPadSnapshot snapshot;
};

static struct NativeInputControllerMapping s_controllerMapping;
static struct NativeInputKeyboardMapping s_keyboardMapping;
static s32 s_controllerToSlotMapping[NATIVE_INPUT_MAX_CONTROLLERS] = {0, -1};

static struct NativeInputController s_controllers[NATIVE_INPUT_MAX_CONTROLLERS];
static const u8 *s_keyboardState;
static s32 s_inputInitialized;
static s32 s_activeKeyboardControllers = 0x1;

extern s32 g_padCommEnable;

static void NativeInput_ResetSnapshot(s32 slot)
{
	struct NativeInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

	snapshot->connected = slot == 0;
	snapshot->status = snapshot->connected ? 0 : NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->id = snapshot->connected ? NATIVE_INPUT_PAD_DIGITAL : NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->buttons = 0xffff;
	snapshot->analog[0] = 0x80;
	snapshot->analog[1] = 0x80;
	snapshot->analog[2] = 0x80;
	snapshot->analog[3] = 0x80;
}

static void NativeInput_WritePadData(s32 slot)
{
	struct NativeInputController *controller = &s_controllers[slot];
	struct NativeInputPadSnapshot *snapshot = &controller->snapshot;
	LPPADRAW pad;

	if (controller->padData == NULL)
		return;

	pad = (LPPADRAW)controller->padData;
	pad->status = snapshot->status;
	pad->id = snapshot->id;
	pad->buttons[0] = (u8)(snapshot->buttons & 0xff);
	pad->buttons[1] = (u8)(snapshot->buttons >> 8);
	pad->analog[0] = snapshot->analog[0];
	pad->analog[1] = snapshot->analog[1];
	pad->analog[2] = snapshot->analog[2];
	pad->analog[3] = snapshot->analog[3];
}

static void NativeInput_DefaultMappings(void)
{
	s_keyboardMapping.kc_square = SDL_SCANCODE_X;
	s_keyboardMapping.kc_circle = SDL_SCANCODE_V;
	s_keyboardMapping.kc_triangle = SDL_SCANCODE_Z;
	s_keyboardMapping.kc_cross = SDL_SCANCODE_C;

	s_keyboardMapping.kc_l1 = SDL_SCANCODE_LSHIFT;
	s_keyboardMapping.kc_l2 = SDL_SCANCODE_LCTRL;
	s_keyboardMapping.kc_l3 = SDL_SCANCODE_LEFTBRACKET;

	s_keyboardMapping.kc_r1 = SDL_SCANCODE_RSHIFT;
	s_keyboardMapping.kc_r2 = SDL_SCANCODE_RCTRL;
	s_keyboardMapping.kc_r3 = SDL_SCANCODE_RIGHTBRACKET;

	s_keyboardMapping.kc_dpad_up = SDL_SCANCODE_UP;
	s_keyboardMapping.kc_dpad_down = SDL_SCANCODE_DOWN;
	s_keyboardMapping.kc_dpad_left = SDL_SCANCODE_LEFT;
	s_keyboardMapping.kc_dpad_right = SDL_SCANCODE_RIGHT;

	s_keyboardMapping.kc_select = SDL_SCANCODE_SPACE;
	s_keyboardMapping.kc_start = SDL_SCANCODE_RETURN;

	s_controllerMapping.gc_square = SDL_CONTROLLER_BUTTON_X;
	s_controllerMapping.gc_circle = SDL_CONTROLLER_BUTTON_B;
	s_controllerMapping.gc_triangle = SDL_CONTROLLER_BUTTON_Y;
	s_controllerMapping.gc_cross = SDL_CONTROLLER_BUTTON_A;

	s_controllerMapping.gc_l1 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
	s_controllerMapping.gc_l2 = SDL_CONTROLLER_AXIS_TRIGGERLEFT | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_l3 = SDL_CONTROLLER_BUTTON_LEFTSTICK;

	s_controllerMapping.gc_r1 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
	s_controllerMapping.gc_r2 = SDL_CONTROLLER_AXIS_TRIGGERRIGHT | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_r3 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;

	s_controllerMapping.gc_dpad_up = SDL_CONTROLLER_BUTTON_DPAD_UP;
	s_controllerMapping.gc_dpad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
	s_controllerMapping.gc_dpad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	s_controllerMapping.gc_dpad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;

	s_controllerMapping.gc_select = SDL_CONTROLLER_BUTTON_BACK;
	s_controllerMapping.gc_start = SDL_CONTROLLER_BUTTON_START;

	s_controllerMapping.gc_axis_left_x = SDL_CONTROLLER_AXIS_LEFTX | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_left_y = SDL_CONTROLLER_AXIS_LEFTY | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_right_x = SDL_CONTROLLER_AXIS_RIGHTX | NATIVE_INPUT_MAP_FLAG_AXIS;
	s_controllerMapping.gc_axis_right_y = SDL_CONTROLLER_AXIS_RIGHTY | NATIVE_INPUT_MAP_FLAG_AXIS;
}

static s32 NativeInput_ControllerButtonState(SDL_GameController *controller, s32 buttonOrAxis)
{
	if (controller == NULL)
		return 0;

	if ((buttonOrAxis & NATIVE_INPUT_MAP_FLAG_AXIS) != 0)
	{
		s32 axis = buttonOrAxis & ~(NATIVE_INPUT_MAP_FLAG_AXIS | NATIVE_INPUT_MAP_FLAG_INVERSE);
		s32 value = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)axis);

		if ((abs(value) > NATIVE_INPUT_AXIS_DEADZONE) && ((buttonOrAxis & NATIVE_INPUT_MAP_FLAG_INVERSE) != 0))
			value *= -1;

		return value;
	}

	if (buttonOrAxis < 0)
		return 0;

	return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)buttonOrAxis) * 32767;
}

static u8 NativeInput_AxisToByte(s32 axis)
{
	s32 value = (axis / 256) + 128;

	if (value < 0)
		return 0;

	if (value > 0xff)
		return 0xff;

	return (u8)value;
}

static void NativeInput_ApplyController(s32 slot)
{
	struct NativeInputController *nativeController = &s_controllers[slot];
	struct NativeInputPadSnapshot *snapshot = &nativeController->snapshot;
	const struct NativeInputControllerMapping *mapping = &s_controllerMapping;
	SDL_GameController *controller = nativeController->controller;
	u16 buttons = 0xffff;

	if ((controller == NULL) || (SDL_GameControllerGetAttached(controller) == 0))
		return;

	snapshot->connected = 1;
	snapshot->status = 0;
	snapshot->id = nativeController->analogEnabled ? NATIVE_INPUT_PAD_ANALOG : NATIVE_INPUT_PAD_DIGITAL;

	if (NativeInput_ControllerButtonState(controller, mapping->gc_square) > 16384)
		buttons &= ~0x8000;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_circle) > 16384)
		buttons &= ~0x2000;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_triangle) > 16384)
		buttons &= ~0x1000;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_cross) > 16384)
		buttons &= ~0x4000;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l1) > 16384)
		buttons &= ~0x400;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r1) > 16384)
		buttons &= ~0x800;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l2) > 16384)
		buttons &= ~0x100;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r2) > 16384)
		buttons &= ~0x200;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_up) > 16384)
		buttons &= ~0x10;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_down) > 16384)
		buttons &= ~0x40;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_left) > 16384)
		buttons &= ~0x80;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_dpad_right) > 16384)
		buttons &= ~0x20;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_l3) > 16384)
		buttons &= ~0x2;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_r3) > 16384)
		buttons &= ~0x4;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_select) > 16384)
		buttons &= ~0x1;
	if (NativeInput_ControllerButtonState(controller, mapping->gc_start) > 16384)
		buttons &= ~0x8;

	if (((buttons & 0x1) == 0) && ((buttons & 0x8) == 0))
	{
		buttons = 0xffff;
		if (nativeController->switchingAnalog == 0)
			nativeController->analogEnabled = nativeController->analogEnabled == 0;
		nativeController->switchingAnalog = 1;
	}
	else
	{
		nativeController->switchingAnalog = 0;
	}

	snapshot->buttons = buttons;
	snapshot->analog[0] = NativeInput_AxisToByte(NativeInput_ControllerButtonState(controller, mapping->gc_axis_right_x));
	snapshot->analog[1] = NativeInput_AxisToByte(NativeInput_ControllerButtonState(controller, mapping->gc_axis_right_y));
	snapshot->analog[2] = NativeInput_AxisToByte(NativeInput_ControllerButtonState(controller, mapping->gc_axis_left_x));
	snapshot->analog[3] = NativeInput_AxisToByte(NativeInput_ControllerButtonState(controller, mapping->gc_axis_left_y));
}

static u16 NativeInput_ReadKeyboard(void)
{
	const struct NativeInputKeyboardMapping *mapping = &s_keyboardMapping;
	u16 buttons = 0xffff;

	if (s_keyboardState == NULL)
		return buttons;

	if (s_keyboardState[mapping->kc_square])
		buttons &= ~0x8000;
	if (s_keyboardState[mapping->kc_circle])
		buttons &= ~0x2000;
	if (s_keyboardState[mapping->kc_triangle])
		buttons &= ~0x1000;
	if (s_keyboardState[mapping->kc_cross])
		buttons &= ~0x4000;
	if (s_keyboardState[mapping->kc_l1])
		buttons &= ~0x400;
	if (s_keyboardState[mapping->kc_l2])
		buttons &= ~0x100;
	if (s_keyboardState[mapping->kc_l3])
		buttons &= ~0x2;
	if (s_keyboardState[mapping->kc_r1])
		buttons &= ~0x800;
	if (s_keyboardState[mapping->kc_r2])
		buttons &= ~0x200;
	if (s_keyboardState[mapping->kc_r3])
		buttons &= ~0x4;
	if (s_keyboardState[mapping->kc_dpad_up])
		buttons &= ~0x10;
	if (s_keyboardState[mapping->kc_dpad_down])
		buttons &= ~0x40;
	if (s_keyboardState[mapping->kc_dpad_left])
		buttons &= ~0x80;
	if (s_keyboardState[mapping->kc_dpad_right])
		buttons &= ~0x20;
	if (s_keyboardState[mapping->kc_select])
		buttons &= ~0x1;
	if (s_keyboardState[mapping->kc_start])
		buttons &= ~0x8;

	return buttons;
}

static s32 NativeInput_KeyboardSuppressed(void)
{
	if (s_keyboardState == NULL)
		return 0;

	return s_keyboardState[SDL_SCANCODE_RALT] || s_keyboardState[SDL_SCANCODE_LALT];
}

static void NativeInput_ApplyKeyboard(s32 slot, u16 keyboardButtons)
{
	struct NativeInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

	if (((s_activeKeyboardControllers & (1 << slot)) == 0) || (keyboardButtons == 0xffff))
		return;

	snapshot->connected = 1;
	snapshot->status = 0;
	snapshot->id = NATIVE_INPUT_PAD_DIGITAL;
	snapshot->buttons &= keyboardButtons;
}

static s32 NativeInput_FindSlotForDeviceIndex(Sint32 deviceIndex)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllerToSlotMapping[slot] == deviceIndex)
			return slot;
	}

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if ((s_controllerToSlotMapping[slot] < 0) && (s_controllers[slot].controller == NULL))
			return slot;
	}

	return -1;
}

static void NativeInput_CloseController(s32 slot)
{
	struct NativeInputController *controller;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
		return;

	controller = &s_controllers[slot];
	if (controller->controller != NULL)
		SDL_GameControllerClose(controller->controller);

	controller->controller = NULL;
	controller->deviceIndex = -1;
	controller->instanceId = -1;
	controller->analogEnabled = 0;
	controller->switchingAnalog = 0;
}

static void NativeInput_OpenController(Sint32 deviceIndex, s32 slot)
{
	struct NativeInputController *controller;
	SDL_Joystick *joystick;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
		return;

	if (SDL_IsGameController(deviceIndex) == 0)
		return;

	controller = &s_controllers[slot];
	if (controller->controller != NULL)
		return;

	controller->controller = SDL_GameControllerOpen(deviceIndex);
	if (controller->controller == NULL)
		return;

	joystick = SDL_GameControllerGetJoystick(controller->controller);
	controller->deviceIndex = deviceIndex;
	controller->instanceId = joystick != NULL ? SDL_JoystickInstanceID(joystick) : -1;
	controller->analogEnabled = 1;
	controller->switchingAnalog = 0;
}

static void NativeInput_OpenKnownControllers(void)
{
	s32 count = SDL_NumJoysticks();
	s32 deviceIndex;

	for (deviceIndex = 0; deviceIndex < count; deviceIndex++)
	{
		s32 slot = NativeInput_FindSlotForDeviceIndex(deviceIndex);

		if (slot >= 0)
			NativeInput_OpenController(deviceIndex, slot);
	}
}

int Platform_InputInit(void)
{
	s32 slot;

	if (s_inputInitialized != 0)
		return 1;

	memset(s_controllers, 0, sizeof(s_controllers));
	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].deviceIndex = -1;
		s_controllers[slot].instanceId = -1;
		NativeInput_ResetSnapshot(slot);
	}

	NativeInput_DefaultMappings();
	s_activeKeyboardControllers = 0x1;
	s_keyboardState = SDL_GetKeyboardState(NULL);

	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
	{
		fprintf(stderr, "[CTR Native] Failed to initialise SDL input subsystem: %s\n", SDL_GetError());
		return 0;
	}

	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	NativeInput_OpenKnownControllers();

	s_inputInitialized = 1;
	return 1;
}

void Platform_InputShutdown(void)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
		NativeInput_CloseController(slot);

	if (s_inputInitialized != 0)
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);

	s_inputInitialized = 0;
	s_keyboardState = NULL;
}

void Platform_InputUpdate(void)
{
	u16 keyboardButtons;
	s32 slot;

	if (s_inputInitialized == 0)
		return;

	if (g_padCommEnable == 0)
		return;

	SDL_PumpEvents();
	keyboardButtons = NativeInput_KeyboardSuppressed() ? 0xffff : NativeInput_ReadKeyboard();

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_ResetSnapshot(slot);
		NativeInput_ApplyController(slot);
		NativeInput_ApplyKeyboard(slot, keyboardButtons);
		NativeInput_WritePadData(slot);
	}
}

void Platform_InputControllerAdded(int deviceIndex)
{
	s32 slot;

	if (s_inputInitialized == 0)
		return;

	slot = NativeInput_FindSlotForDeviceIndex(deviceIndex);
	if (slot >= 0)
		NativeInput_OpenController(deviceIndex, slot);
}

void Platform_InputControllerRemoved(int instanceId)
{
	s32 slot;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		if (s_controllers[slot].instanceId == instanceId)
		{
			NativeInput_CloseController(slot);
			return;
		}
	}
}

int Platform_InputCycleKeyboardController(void)
{
	s_activeKeyboardControllers++;
	s_activeKeyboardControllers %= 4;
	if (s_activeKeyboardControllers == 0)
		s_activeKeyboardControllers++;

	return s_activeKeyboardControllers;
}

void Platform_InputPadInit(int slot, unsigned char *padData)
{
	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
		return;

	s_controllers[slot].padData = padData;
	NativeInput_ResetSnapshot(slot);
	NativeInput_WritePadData(slot);
}

int Platform_InputPadGetState(int port)
{
	s32 mtap = port & 3;
	s32 slot = ((mtap * 2) + (port >> 4)) & 1;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS))
		return PadStateDiscon;

	return s_controllers[slot].snapshot.connected ? PadStateStable : PadStateDiscon;
}

void Platform_InputPadVibrate(int port, unsigned char *table, int len)
{
	s32 mtap = port & 3;
	s32 slot = ((mtap * 2) + (port >> 4)) & 1;
	struct NativeInputController *controller;
	u16 freqHigh;
	u16 freqLow;

	if ((slot < 0) || (slot >= NATIVE_INPUT_MAX_CONTROLLERS) || (table == NULL) || (len <= 0))
		return;

	controller = &s_controllers[slot];
	if (controller->controller == NULL)
		return;

	freqHigh = table[0] * 255;
	freqLow = len > 1 ? table[1] * 255 : 0;

	if ((freqLow != 0) && (freqLow < 4096))
		freqLow = 4096;

	if ((freqHigh != 0) && (freqHigh < 4096))
		freqHigh = 4096;

	SDL_GameControllerRumble(controller->controller, freqLow, freqHigh, 200);
}
