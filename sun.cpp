#include "sun.h"
#include <stdio.h>
#include <math.h>

real64 Counter = 0.0;

void InitArena(memory_arena *Arena, uint64 Capacity, void *BasePtr)
{
    Arena->BasePtr = (uint8*)BasePtr;
    Arena->Capacity = Capacity;
    Arena->Size = 0;
}

#define PushArenaData(Arena, PODType) _PushArenaData((Arena), sizeof(PODType))
void *_PushArenaData(memory_arena *Arena, uint64 Size)
{
    Assert(Arena->Size + Size <= Arena->Capacity);
    void *MemoryPtr = Arena->BasePtr + Arena->Size;
    Arena->Size += Size;

    return (void*)MemoryPtr;
}

void FillAudioBuffer(tmp_sound_data *SoundData)
{
    uint32 ToneHz = 440;
    uint32 ALen = 40 * ToneHz;

    SoundData->SoundBufferSize = ALen;
    uint32 Size = SoundData->SoundBufferSize;

    for(uint32 i = 0; i < ALen; ++i)
    {
        real32 Angle = 2.f * M_PI * i / (real32)ToneHz;
        uint16 Value = 10000 * sinf(Angle);
        // NOTE = Temp : no sound
        Value = 0;
        SoundData->SoundBuffer[i] = Value;
    }
}

void GameInitialization(game_memory *Memory)
{
    // Init Scratch Pool
    InitArena(&Memory->ScratchArena, Memory->ScratchMemPoolSize, Memory->ScratchMemPool);

    // Push sound buffer on it for use
    tmp_sound_data *SoundBuffer = (tmp_sound_data*)PushArenaData(&Memory->ScratchArena, tmp_sound_data);

    game_state *State = (game_state*)Memory->PermanentMemPool;
    State->SoundData = SoundBuffer;

    FillAudioBuffer(SoundBuffer);
    SoundBuffer->ReloadSoundBuffer = true;

    Memory->IsInitialized = true;
}

extern "C" GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsInitialized)
    {
        GameInitialization(Memory);
    }

    game_state *State = (game_state*)Memory->PermanentMemPool;
    if(Input->KeyReleased)
    {
        tmp_sound_data *SoundData = State->SoundData;
        FillAudioBuffer(SoundData);
        SoundData->ReloadSoundBuffer = true;
    }

    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        DebugPrint("%g, Mouse: %d,%d\n", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Counter = 0.0;
    }

}
