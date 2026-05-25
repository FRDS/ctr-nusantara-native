#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041e20-0x80041e9c.
void PROC_DestroyObject(void *object, int threadFlags)
{
	struct JitPool *myPool;

	if (object == NULL)
		return;

	if ((threadFlags & 0x300) == 0x100)
		myPool = &sdata->gGT->JitPools.largeStack;
	else if ((threadFlags & 0x300) == 0x200)
		myPool = &sdata->gGT->JitPools.mediumStack;
	else
		myPool = &sdata->gGT->JitPools.smallStack;

	// in allocation, "next" and "prev" are abstracted
	// with obj+=8, so not all structs need "next" and "prev",
	// now subtract 8 bytes to access those two pointers
	object = (void *)((u8 *)object - 8);

	// add object back to free list
	LIST_AddFront(&myPool->free, (struct Item *)object);
}
