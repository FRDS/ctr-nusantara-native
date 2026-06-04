#include "platform/native_replay_scheduler.h"

#if defined(CTR_INTERNAL)
void NativeReplayScheduler_ConfigureFromArgs(int argc, char **argv)
{
	(void)argc;
	(void)argv;
}

void NativeReplayScheduler_BeginFrame(s32 frameTimer, s32 frameCounter, s32 mainGameState, s32 loadingStage, s32 levelID)
{
	(void)frameTimer;
	(void)frameCounter;
	(void)mainGameState;
	(void)loadingStage;
	(void)levelID;
}

int NativeReplayScheduler_EndFrame(s32 frameTimer, s32 frameCounter, s32 mainGameState, s32 loadingStage, s32 levelID)
{
	(void)frameTimer;
	(void)frameCounter;
	(void)mainGameState;
	(void)loadingStage;
	(void)levelID;
	return 0;
}
#endif
