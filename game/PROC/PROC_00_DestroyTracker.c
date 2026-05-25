#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041dc0-0x80041dfc.
void PROC_DestroyTracker(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT->numMissiles > 0)
		gGT->numMissiles--;

	PROC_DestroyInstance(t);
}
