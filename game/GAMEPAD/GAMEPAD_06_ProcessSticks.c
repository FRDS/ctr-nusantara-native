#include <common.h>

static int GAMEPAD_ProcessSticks_IsAnalogLike(u8 controllerData)
{
	return controllerData == ((PAD_ID_ANALOG_STICK << 4) | 3) || controllerData == ((PAD_ID_ANALOG << 4) | 3) || controllerData == ((PAD_ID_NEGCON << 4) | 3) ||
	       controllerData == ((PAD_ID_JOGCON << 4) | 3);
}

static s16 GAMEPAD_ProcessSticks_StepTowardZero(s16 value)
{
	int step = value;

	if (step > 0)
	{
		step -= 0xff;
		if (step < 0)
			step = 0;
	}
	else
	{
		step += 0xff;
		if (step > 0)
			step = 0;
	}

	return step;
}

static s16 GAMEPAD_ProcessSticks_StepTowardMax(s16 value)
{
	int step = value;

	if (step >= 0x100)
	{
		step -= 0xff;
		if (step < 0xff)
			step = 0xff;
	}
	else
	{
		step += 0xff;
		if (step >= 0x100)
			step = 0xff;
	}

	return step;
}

static s16 GAMEPAD_ProcessSticks_StepTowardCenter(s16 value)
{
	int step = value;

	if (step >= 0x81)
	{
		step -= 0xff;
		if (step < 0x80)
			step = 0x80;
	}
	else
	{
		step += 0xff;
		if (step >= 0x81)
			step = 0x80;
	}

	return step;
}

static s16 GAMEPAD_ProcessSticks_ResolveAxis(s16 axis, s16 rawAxis, int held, int negativeButton, int positiveButton, int useRaw)
{
	if ((held & negativeButton) != 0)
		return GAMEPAD_ProcessSticks_StepTowardZero(axis);

	if ((held & positiveButton) != 0)
		return GAMEPAD_ProcessSticks_StepTowardMax(axis);

	if (useRaw)
		return rawAxis;

	return GAMEPAD_ProcessSticks_StepTowardCenter(axis);
}

static void GAMEPAD_ProcessSticks_ResetRaw(struct GamepadBuffer *pad)
{
	pad->stickLX_dontUse1 = 0x80;
	pad->stickLY_dontUse1 = 0x80;
	pad->stickRX = 0x80;
	pad->stickRY = 0x80;
}

static void GAMEPAD_ProcessSticks_ResetRawAndResolved(struct GamepadBuffer *pad)
{
	GAMEPAD_ProcessSticks_ResetRaw(pad);
	pad->stickLX = 0x80;
	pad->stickLY = 0x80;
}

static void GAMEPAD_ProcessSticks_CheckIdleAxis(struct GamepadBuffer *pad, s16 axis)
{
	int delta = axis - 0x80;

	if (delta < 0)
		delta = -delta;

	if (delta >= 0x31)
		pad->framesSinceLastInput = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80025854-0x80025d10.
void GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads)
{
	u8 controllerData;
	int iVar4;
	int iVar7;
	s16 sVar8;

	struct GamepadBuffer *pad;
	struct ControllerPacket *packet;
	struct RacingWheelData *rwd = &data.rwd[0];
	int i;

	for (pad = &gGamepads->gamepad[0], i = 0; i < 8; pad++, i++, rwd = (struct RacingWheelData *)((char *)rwd + sizeof(struct RacingWheelData)))
	{
		packet = pad->ptrControllerPacket;
		pad->rwd = NULL;

		if (packet == NULL)
		{
			GAMEPAD_ProcessSticks_ResetRawAndResolved(pad);
			continue;
		}

		if (packet->plugged == PLUGGED)
		{
			controllerData = packet->controllerData;

			if ((controllerData == ((PAD_ID_ANALOG_STICK << 4) | 3)) || (controllerData == ((PAD_ID_ANALOG << 4) | 3)))
			{
				pad->stickLX_dontUse1 = packet->analog.leftX;

				if (packet->analog.leftY == 0xff && pad->unk_1 != 0xff)
					pad->stickLY_dontUse1 = pad->unk_1;
				else
					pad->stickLY_dontUse1 = packet->analog.leftY;

				pad->unk_1 = packet->analog.leftY;
				pad->stickRX = packet->analog.rightX;
				pad->stickRY = packet->analog.rightY;
			}

			else if (controllerData == ((PAD_ID_NEGCON << 4) | 3))
			{
				if (i < 4)
					pad->rwd = rwd;

				pad->stickLX_dontUse1 = packet->neGcon.twist;
				pad->stickLY_dontUse1 = 0x80;
				pad->stickRX = 0x80;
				pad->stickRY = 0x80;
			}

			else if (controllerData == ((PAD_ID_JOGCON << 4) | 3))
			{
				if (i < 4)
					pad->rwd = rwd;

				sVar8 = packet->jogcon.jog_rot;
				iVar4 = (int)sVar8;

				if (iVar4 < 0)
				{
					iVar7 = ((-10 - iVar4) - rwd->deadZone) * 8;
					if (iVar7 < 0)
						iVar7 = 0;
					if (iVar7 > 0xff)
						iVar7 = 0xff;

					sVar8 += 0x80;
					if (iVar4 < -0x80)
					{
						sVar8 = -0x80;
						sVar8 += 0x80;
					}
				}
				else
				{
					iVar7 = ((iVar4 - 10) - rwd->deadZone) * 8;
					if (iVar7 < 0)
						iVar7 = 0;
					if (iVar7 > 0xff)
						iVar7 = 0xff;

					sVar8 += 0x80;
					if (0x7f < iVar4)
					{
						sVar8 = 0x7f;
						sVar8 += 0x80;
					}
				}
				pad->unk43 = (char)iVar7;
				pad->stickLX_dontUse1 = sVar8;
				pad->stickLY_dontUse1 = 0x80;
				pad->stickRX = 0x80;
				pad->stickRY = 0x80;
			}

			else
			{
				GAMEPAD_ProcessSticks_ResetRaw(pad);
			}
		}

		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickLX_dontUse1);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickLY_dontUse1);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRX);
		GAMEPAD_ProcessSticks_CheckIdleAxis(pad, pad->stickRY);

		controllerData = packet->controllerData;
		iVar4 = GAMEPAD_ProcessSticks_IsAnalogLike(controllerData);
		iVar7 = pad->buttonsHeldCurrFrame;

		pad->stickLX = GAMEPAD_ProcessSticks_ResolveAxis(pad->stickLX, pad->stickLX_dontUse1, iVar7, BTN_LEFT, BTN_RIGHT, iVar4);
		pad->stickLY = GAMEPAD_ProcessSticks_ResolveAxis(pad->stickLY, pad->stickLY_dontUse1, iVar7, BTN_UP, BTN_DOWN, iVar4);
	}
}
