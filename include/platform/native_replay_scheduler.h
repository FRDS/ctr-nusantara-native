#ifndef PLATFORM_NATIVE_REPLAY_SCHEDULER_H
#define PLATFORM_NATIVE_REPLAY_SCHEDULER_H

#include <macros.h>

#if defined(CTR_INTERNAL)
void NativeReplayScheduler_ConfigureFromArgs(int argc, char **argv);
void NativeReplayScheduler_BeginFrame(s32 frameTimer, s32 frameCounter, s32 mainGameState, s32 loadingStage, s32 levelID);
int NativeReplayScheduler_EndFrame(s32 frameTimer, s32 frameCounter, s32 mainGameState, s32 loadingStage, s32 levelID);
#endif

#endif
