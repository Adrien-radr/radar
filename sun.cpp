#include "sun.h"
#include <stdio.h>
#include <math.h>

real64 Counter = 0.0;

void FillAudioBuffer(game_state *State)
{
    uint32 ToneHz = 440;
    uint32 ALen = 40 * ToneHz;

    State->SoundBufferSize = ALen;
    uint32 Size = State->SoundBufferSize;

    for(uint32 i = 0; i < ALen; ++i)
    {
        real32 Angle = 2.f * M_PI * i / (real32)ToneHz;
        uint16 Value = 10000 * sinf(Angle);
        // NOTE = Temp : no sound
        Value = 0;
        State->SoundBuffer[i] = Value;
    }
}

void GameInitialization(game_memory *Memory)
{
    Assert(sizeof(game_state) <= Memory->ScratchMemPoolSize);
    game_state *State = (game_state*)Memory->ScratchMemPool;

    FillAudioBuffer(State);
    State->ReloadSoundBuffer = true;

    Memory->IsInitialized = true;
}

extern "C" GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsInitialized)
    {
        GameInitialization(Memory);
    }

    game_state *State = (game_state*)Memory->ScratchMemPool;
    if(Input->KeyReleased)
    {
        FillAudioBuffer(State);
        State->ReloadSoundBuffer = true;
    }


    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        printf("%g, Mouse: %d,%d\n", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Counter = 0.0;
    }

}
