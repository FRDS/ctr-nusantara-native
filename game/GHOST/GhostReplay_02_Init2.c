#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80027b88-0x80027df4.
void GhostReplay_Init2(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Thread *thread = gGT->threadBuckets[GHOST].thread;
	struct Driver *driver;
	struct GhostTape *tape;
	int ghostID;
	int characterIndex;
	int characterID;
	int timeTrialFlags;
	struct Instance *inst;
	struct Model *model;
	char *name;

	for (; thread != NULL; thread = thread->siblingThread)
	{
		driver = thread->object;
		if (driver == NULL)
			continue;

		tape = driver->ghostTape;
		if (tape->ptrEnd == tape->ptrStart)
			continue;

		ghostID = driver->ghostID;
		if (ghostID == 0)
		{
			if (sdata->boolReplayHumanGhost == 0)
				continue;
		}
		else
		{
			if (ghostID != 1)
				continue;

			timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			if ((timeTrialFlags & 1) == 0)
				continue;
		}

		tape->timeElapsedInRace = 0;
		tape->timeInPacket32_backup = 0;
		tape->unk20 = 0;
		tape->timeInPacket32 = 0;
		tape->timeInPacket01 = 0;
		tape->ptrCurr = tape->ptrStart;

		sdata->boolGhostsDrawing = 1;
		driver->ghostBoolInit = 1;
		driver->ghostBoolStarted = 0;

		characterIndex = ghostID + 1;
		if (ghostID != 0)
		{
			timeTrialFlags = sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			if ((timeTrialFlags & 2) != 0)
				characterIndex = ghostID + 2;
		}

		characterID = data.characterIDs[characterIndex];
		model = VehBirth_GetModelByName(data.MetaDataCharacters[characterID].name_Debug);

		driver->wheelSize = 0;
		if (characterID != NITROS_OXIDE)
			driver->wheelSize = 0xccc;

		inst = driver->instSelf;
		name = sdata->s_ghost0;
		if (ghostID != 0)
			name = sdata->s_ghost1;

		INSTANCE_Birth(inst, model, name, inst->thread, 7);
		GhostReplay_ThTick(thread);

		tape->unk2[0] = tape->unk1[0];
		tape->unk2[1] = tape->unk1[1];
		tape->unk2[2] = tape->unk1[2];
		tape->unk4[0] = tape->unk3[0];
		tape->unk4[1] = tape->unk3[1];
		tape->unk4[2] = tape->unk3[2];
		tape->unk20 = 0;
	}
}
