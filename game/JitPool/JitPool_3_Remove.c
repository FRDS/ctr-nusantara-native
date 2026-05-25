#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003112c-0x8003116c.
void JitPool_Remove(struct JitPool *AP, struct Item *item)
{
	LIST_RemoveMember(&AP->taken, item);
	LIST_AddFront(&AP->free, item);
}
