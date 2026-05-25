#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041e9c-0x80041f04.
void PROC_DestroySelf(struct Thread *t)
{
	// thread must exist
	if (t == 0)
		return;

	// this is usuallly PROC_DestroyInstance
	if (t->funcThDestroy != 0)
		t->funcThDestroy(t);

	// used by RB_Follower
	t->timesDestroyed++;

	// destroy object attached,
	// guaranteed all threads have one
	PROC_DestroyObject(t->object, t->flags);

	// recycle thread
	LIST_AddFront(&sdata->gGT->JitPools.thread.free, (struct Item *)t);
}
