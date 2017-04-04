#include "sun.h"
#include <stdio.h>
#include <string.h>
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
        uint16 Value = (uint16)(10000 * sinf(Angle));
        // NOTE = Temp : no sound
        Value = 0;
        SoundData->SoundBuffer[i] = Value;
    }
}

void GameInitialization(game_memory *Memory)
{
    // Init Scratch Pool
    InitArena(&Memory->ScratchArena, Memory->ScratchMemPoolSize, Memory->ScratchMemPool);

    // Push GameSystem Data
    tmp_sound_data *SoundBuffer = (tmp_sound_data*)PushArenaData(&Memory->ScratchArena, tmp_sound_data);
    console_log *ConsoleLog = (console_log*)PushArenaData(&Memory->ScratchArena, console_log);

    game_system *System = (game_system*)Memory->PermanentMemPool;
    System->ConsoleLog = ConsoleLog;
    System->SoundData = SoundBuffer;

    FillAudioBuffer(SoundBuffer);
    SoundBuffer->ReloadSoundBuffer = true;

    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);
    State->PlayerPosition = vec3f(300, 300, 0);

    game_camera &Camera = State->Camera;
    Camera.Position = vec3f(10,10,10); // TODO - From Config
    Camera.Target = vec3f(0,0,0); // TODO - From Config
    Camera.Up = vec3f(0,0,1);
    Camera.Forward = Normalize(Camera.Target - Camera.Position);
    Camera.Right = Normalize(Cross(Camera.Forward, Camera.Up));
    Camera.Up = Normalize(Cross(Camera.Right, Camera.Forward));
    Camera.LinearSpeed = 20; // TODO - From Config
    Camera.AngularSpeed = 30; // TODO - From Config
    Camera.SpeedMult = 2.0f; // TODO - From Config
    Camera.SpeedMode = 0;
    Camera.FreeflyMode = false;

    vec2f Azimuth = Normalize(vec2f(Camera.Forward[0], Camera.Forward[1]));
    Camera.Phi = atan2f(Azimuth.y, Azimuth.x);
    Camera.Theta = atan2f(Camera.Forward.z, sqrtf(Dot(Azimuth, Azimuth)));

    Memory->IsInitialized = true;
}

void MovePlayer(game_state *State, game_input *Input)
{
    game_camera &Camera = State->Camera;

    vec3f CameraMove(0, 0, 0);
    if(KEY_DOWN(Input->KeyW)) CameraMove += Camera.Forward;
    if(KEY_DOWN(Input->KeyS)) CameraMove -= Camera.Forward;
    if(KEY_DOWN(Input->KeyA)) CameraMove -= Camera.Right;
    if(KEY_DOWN(Input->KeyD)) CameraMove += Camera.Right;

    if(KEY_HIT(Input->KeyLShift))      Camera.SpeedMode += 1;
    else if(KEY_UP(Input->KeyLShift))  Camera.SpeedMode -= 1;
    if(KEY_HIT(Input->KeyLCtrl))       Camera.SpeedMode -= 1;
    else if(KEY_UP(Input->KeyLCtrl))   Camera.SpeedMode += 1;

    Normalize(CameraMove);
    real32 SpeedMult = Camera.SpeedMode ? (Camera.SpeedMode > 0 ? Camera.SpeedMult : 1.0f / Camera.SpeedMult) : 1.0f;
    CameraMove *= (real32)(Input->dTime * Camera.LinearSpeed * SpeedMult);
    Camera.Position += CameraMove;
    Camera.Target += CameraMove;

    vec3f Move;
    Move.x = (real32)Input->MousePosX;
    Move.y = (real32)(540-Input->MousePosY);

    if(Move.x < 0) Move.x = 0;
    if(Move.y < 0) Move.y = 0;
    if(Move.x >= 960) Move.x = 959;
    if(Move.y >= 540) Move.y = 539;

    State->PlayerPosition = Move;
}

void LogString(console_log *Log, char const *String)
{
    // NOTE - Have an exposed Queue of Console String on Platform
    // The Game sends Console Log strings to that queue
    // The platform process the queue in order each frame and draws them in console
    memcpy(Log->MsgStack[Log->WriteIdx], String, ConsoleLogStringLen);
    Log->WriteIdx = (Log->WriteIdx + 1) % ConsoleLogCapacity;

    if(Log->StringCount >= ConsoleLogCapacity)
    {
        Log->ReadIdx = (Log->ReadIdx + 1) % ConsoleLogCapacity;
    }
    else
    {
        Log->StringCount++;
    }
}

DLLEXPORT GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsInitialized)
    {
        GameInitialization(Memory);
    }

    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

#if 0
    if(Input->KeyReleased)
    {
        tmp_sound_data *SoundData = System->SoundData;
        FillAudioBuffer(SoundData);
        SoundData->ReloadSoundBuffer = true;
    }
#endif

    MovePlayer(State, Input);

    Counter += Input->dTime;

    if(Counter > 0.75)
    {
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        LogString(System->ConsoleLog, Msg);
        Counter = 0.0;
    }

}
