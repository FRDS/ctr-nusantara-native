#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041f04-0x80041f58.
void PROC_DestroyBloodline(struct Thread *t)
{
	while (t != 0)
	{
		struct Thread *siblingThread;

		// recursively find all children
		if (t->childThread != 0)
			PROC_DestroyBloodline(t->childThread);

		siblingThread = t->siblingThread;
		PROC_DestroySelf(t);
		t = siblingThread;
	}
}
