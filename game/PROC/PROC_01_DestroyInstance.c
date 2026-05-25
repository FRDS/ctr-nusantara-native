#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041dfc-0x80041e20.
void PROC_DestroyInstance(struct Thread *t)
{
	INSTANCE_Death(t->inst);
}
