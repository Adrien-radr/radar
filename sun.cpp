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

#define PushArenaStruct(Arena, Struct) _PushArenaData((Arena), sizeof(Struct))
#define PushArenaData(Arena, Size) _PushArenaData((Arena), (Size))
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

void InitCamera(game_camera *Camera, game_memory *Memory)
{
    game_config const &Config = Memory->Config;

    Camera->Position = Config.CameraPosition;
    Camera->Target = Config.CameraTarget;
    Camera->Up = vec3f(0,1,0);
    Camera->Forward = Normalize(Camera->Target - Camera->Position);
    Camera->Right = Normalize(Cross(Camera->Forward, Camera->Up));
    Camera->Up = Normalize(Cross(Camera->Right, Camera->Forward));
    Camera->LinearSpeed = Config.CameraSpeedBase;
    Camera->AngularSpeed = Config.CameraSpeedAngular;
    Camera->SpeedMult = Config.CameraSpeedMult;
    Camera->SpeedMode = 0;
    Camera->FreeflyMode = false;

    vec2f Azimuth = Normalize(vec2f(Camera->Forward[0], Camera->Forward[2]));
    Camera->Phi = atan2f(Azimuth.y, Azimuth.x);
    Camera->Theta = atan2f(Camera->Forward.y, sqrtf(Dot(Azimuth, Azimuth)));
}

void GameInitialization(game_memory *Memory)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    // Init Scratch Pool
    InitArena(&Memory->ScratchArena, Memory->ScratchMemPoolSize, Memory->ScratchMemPool);

    // Push Data
    tmp_sound_data *SoundBuffer = (tmp_sound_data*)PushArenaStruct(&Memory->ScratchArena, tmp_sound_data);
    console_log *ConsoleLog = (console_log*)PushArenaStruct(&Memory->ScratchArena, console_log);
    water_system *WaterSystem = (water_system*)PushArenaStruct(&Memory->ScratchArena, water_system);
    size_t WaterVertexDataSize = Square(State->WaterSubdivs) * (4 + 4 + 2) * sizeof(vec3f); // Pos(4) + Nrm(4) + FaceNrm(2)
    size_t WaterVertexPositionsSize = Square(State->WaterSubdivs) * 4 * sizeof(vec3f);
    real32 *WaterVertexData = (real32*)PushArenaData(&Memory->ScratchArena, WaterVertexDataSize);

    System->ConsoleLog = ConsoleLog;
    System->SoundData = SoundBuffer;
    System->WaterSystem = WaterSystem;
    System->WaterSystem->VertexDataSize = WaterVertexDataSize;
    System->WaterSystem->VertexPositionsSize = WaterVertexPositionsSize;
    System->WaterSystem->VertexData = WaterVertexData;
    System->WaterSystem->Positions = System->WaterSystem->VertexData;
    System->WaterSystem->Normals = System->WaterSystem->VertexData + WaterVertexPositionsSize;
    System->WaterSystem->FaceNormals = System->WaterSystem->VertexData + 2 * WaterVertexPositionsSize;

    FillAudioBuffer(SoundBuffer);
    SoundBuffer->ReloadSoundBuffer = true;

    State->DisableMouse = false;
    State->PlayerPosition = vec3f(300, 300, 0);

    InitCamera(&State->Camera, Memory);

    State->LightColor = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
    State->WaterCounter = 0.0;

    Memory->IsInitialized = true;
}

void MovePlayer(game_state *State, game_input *Input)
{
    game_camera &Camera = State->Camera;
    vec2i MousePos = vec2i(Input->MousePosX, Input->MousePosY);

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

    if(MOUSE_HIT(Input->MouseRight))
    {
        Camera.FreeflyMode = true;
        State->DisableMouse = true;
        Camera.LastMousePos = MousePos;
    }
    if(MOUSE_UP(Input->MouseRight))
    {
        Camera.FreeflyMode = false;
        State->DisableMouse = false;
    }

    if(Camera.FreeflyMode)
    {
        vec2i MouseOffset = MousePos - Camera.LastMousePos;
        Camera.LastMousePos = MousePos;

        if(MouseOffset.x != 0 || MouseOffset.y != 0)
        {
            Camera.Phi += MouseOffset.x * Input->dTime * Camera.AngularSpeed;
            Camera.Theta -= MouseOffset.y * Input->dTime * Camera.AngularSpeed;

            if(Camera.Phi > M_TWO_PI) Camera.Phi -= M_TWO_PI;
            if(Camera.Phi < 0.0f) Camera.Phi += M_TWO_PI;

            Camera.Theta = max(-M_PI_OVER_TWO + 1e-5f, min(M_PI_OVER_TWO - 1e-5f, Camera.Theta));
            real32 CosTheta = cosf(Camera.Theta);
            Camera.Forward.x = CosTheta * cosf(Camera.Phi);
            Camera.Forward.y = sinf(Camera.Theta);
            Camera.Forward.z = CosTheta * sinf(Camera.Phi);

            Camera.Right = Normalize(Cross(Camera.Forward, vec3f(0, 1, 0)));
            Camera.Up = Normalize(Cross(Camera.Right, Camera.Forward));
        }
    }

    Camera.Target = Camera.Position + Camera.Forward;

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

float WaveFreq(float Start, float Freq, float Idx, real64 dTime)
{
    return sinf(Idx*Start + Freq * (real32)dTime);
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

    State->LightColor = vec4f(1.0f, 1.0f, 1.0f, 1.0f);

    State->WaterCounter += Input->dTime;
    //if(State->WaterCounter >= (1.0)) State->WaterCounter -= 1.0;
    vec3f *WaterPositions = (vec3f*)System->WaterSystem->Positions;
    vec3f *WaterNormals = (vec3f*)System->WaterSystem->Normals;
    vec3f *WaterFaceNormals = (vec3f*)System->WaterSystem->FaceNormals;

    vec2f SubdivDim = State->WaterWidth / (real32)State->WaterSubdivs;
    float Start = 0.10f;
    float Speed = 0.15f;
    float Magnitude = 2.f;
    {
        // Simulation step on the positions
        for(uint32 j = 0; j < State->WaterSubdivs; ++j)
        {
            for(uint32 i = 0; i < State->WaterSubdivs; ++i)
            {
                uint32 Idx = j*State->WaterSubdivs+i;

                float ti = WaveFreq(Start, Speed, i, State->WaterCounter);
                float ti1 = WaveFreq(Start, Speed, (i+1), State->WaterCounter);
                float tj = WaveFreq(1.1f*Start, 5.f*Speed, j, State->WaterCounter);
                float tj1 = WaveFreq(1.1f*Start, 5.f*Speed, (j+1), State->WaterCounter);

                WaterPositions[Idx*4+0] = vec3f(i*SubdivDim.x, Magnitude * ti*tj, j*SubdivDim.y);
                WaterPositions[Idx*4+1] = vec3f(i*SubdivDim.x, Magnitude * ti*tj1, (j+1)*SubdivDim.y);
                WaterPositions[Idx*4+2] = vec3f((i+1)*SubdivDim.x, Magnitude * ti1*tj1, (j+1)*SubdivDim.y);
                WaterPositions[Idx*4+3] = vec3f((i+1)*SubdivDim.x, Magnitude * ti1*tj, j*SubdivDim.y);

                WaterFaceNormals[Idx*2+0] = Cross(WaterPositions[Idx*4+1] - WaterPositions[Idx*4+0],
                        WaterPositions[Idx*4+2] - WaterPositions[Idx*4+0]);
                WaterFaceNormals[Idx*2+1] = Cross(WaterPositions[Idx*4+2] - WaterPositions[Idx*4+0],
                        WaterPositions[Idx*4+3] - WaterPositions[Idx*4+0]);
            }
        }
        // Regenerate normals
        int32 IdxOffset[] = { 0, State->WaterSubdivs, State->WaterSubdivs + 1, 1 };
        for(uint32 j = 0; j < State->WaterSubdivs; ++j)
        {
            for(uint32 i = 0; i < State->WaterSubdivs; ++i)
            {
                int32 Idx = j * State->WaterSubdivs + i;
                for(uint32 v = 0; v < 4; ++v)
                {
                    int32 FaceIdx = Idx + IdxOffset[v];
                    int32 IdxTL = FaceIdx - State->WaterSubdivs - 1;
                    int32 IdxTR = FaceIdx - State->WaterSubdivs;
                    int32 IdxBL = FaceIdx - 1;
                    int32 IdxBR = FaceIdx;

                    // Add Face normals from the 4 surrounding quads
                    vec3f Sum(0.f);

                    int32 iIdx = FaceIdx % State->WaterSubdivs;
                    int32 jIdx = FaceIdx / State->WaterSubdivs;
                    if(iIdx > 0)
                    {
                        if(jIdx > 0)
                        {
                            Sum += WaterFaceNormals[IdxTL*2+0] + WaterFaceNormals[IdxTL*2+1];
                        }
                        if(jIdx < (State->WaterSubdivs-1))
                        {
                            Sum += WaterFaceNormals[IdxBL*2+1];
                        }
                    }
                    if(iIdx < (State->WaterSubdivs-1))
                    {
                        if(jIdx > 0)
                        {
                            Sum += WaterFaceNormals[IdxTR*2+0];
                        }
                        if(jIdx < (State->WaterSubdivs-1))
                        {
                            Sum += WaterFaceNormals[IdxBR*2+0] + WaterFaceNormals[IdxBR*2+1];
                        }
                    }
                    WaterNormals[Idx*4+v] = Normalize(Sum);
                }
            }
        }
    }

    if(Counter > 0.75)
    {
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        LogString(System->ConsoleLog, Msg);
        Counter = 0.0;
    }

}
