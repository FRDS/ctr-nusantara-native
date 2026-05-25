#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041f58-0x80041ff4.
void PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th)
{
	while (th != 0)
	{
		struct Thread *siblingThread = th->siblingThread;

		// if this thread is alive
		if ((th->flags & 0x800) == 0)
		{
			// [wow this sounds dark]
			// check child's bloodline, and if child is dead, then
			// "childThread" inside "th" will be replaced with the
			// child's sibling, or nullptr if all children are dead

			// recursively find all children
			if (th->childThread != 0)
				PROC_CheckBloodlineForDead(&th->childThread, th->childThread);

			// current thread is alive, doesn't need to be overwritten,
			// next check sibling, so sibling will be replaced by the
			// sibling's sibling, if the sibling's sibling is dead
			replaceSelf = &th->siblingThread;
		}

		// if this thread needs to die
		else
		{
			// recursively find all children
			if (th->childThread != 0)
				PROC_DestroyBloodline(th->childThread);

			PROC_DestroySelf(th);

			// replace thread with pointer to it's own sibling
			*replaceSelf = siblingThread;

			// dont overwrite replaceSelf like in previous
			// "if" block, cause the next dead sibling can
			// still take the place in replaceSelf
		}

		th = siblingThread;
	}
}
