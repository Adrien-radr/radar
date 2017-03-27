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

    State->PlayerPosition = vec3f(300, 300, 0);

    Memory->IsInitialized = true;
}

void MovePlayer(game_state *State, game_input *Input)
{
    vec3f Move(0, 0, 0);
    if(KEY_DOWN(Input->KeyW))
    {
        Move.y += 1;
    }
    if(KEY_DOWN(Input->KeyS))
    {
        Move.y -= 1;
    }
    if(KEY_DOWN(Input->KeyA))
    {
        Move.x -= 1;
    }
    if(KEY_DOWN(Input->KeyD))
    {
        Move.x += 1;
    }


    Normalize(Move);
    Move *= Input->dTime * 100.f;

    Move.x = Input->MousePosX;
    Move.y = 540-Input->MousePosY;

    if(Move.x < 0) Move.x = 0;
    if(Move.y < 0) Move.y = 0;
    if(Move.x >= 960) Move.x = 959;
    if(Move.y >= 540) Move.y = 539;

    State->PlayerPosition = Move;
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

    MovePlayer(State, Input);

    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        DebugPrint("%g, Mouse: %d,%d\n", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Counter = 0.0;
    }

}
