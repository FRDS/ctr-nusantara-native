#include <common.h>

// search starts with driver thread's child
// searches for turbo model
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042394-0x800423fc.
struct Thread *PROC_SearchForModel(struct Thread *th, s16 modelID)
{
	while (th != 0)
	{
		// if found, quit
		if (th->modelIndex == modelID)
			return th;

		// check children recursively, quit if found
		struct Thread *other = PROC_SearchForModel(th->childThread, modelID);
		if (other != 0)
			return other;

		th = th->siblingThread;
	}

	return th;
}
