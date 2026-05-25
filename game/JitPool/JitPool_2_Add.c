#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800310d4-0x8003112c.
int JitPool_Add(struct JitPool *AP)
{
	struct Item *item;
	item = AP->free.first;

	if (item != 0)
	{
		LIST_RemoveMember(&AP->free, item);
		LIST_AddFront(&AP->taken, item);
	}

	return (int)item;
}
