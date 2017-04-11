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

// TODO - Should return a vec2f, with complex having vec2f cast
// TODO - Use precomputed and better random variables, not rand()
#include <stdlib.h>
complex GaussianRandomVariable()
{
    float U = rand()/(real32)RAND_MAX;
    float V = rand()/(real32)RAND_MAX;

    float Pre = sqrtf(-2.f * logf(U));
    float TwoPiV = V * M_TWO_PI;

    return complex(Pre * cosf(TwoPiV), Pre * sinf(TwoPiV));
}

real32 Phillips(int n_prime, int m_prime)
{
    vec2f K(M_PI * (2.f * n_prime - g_WaterN) / g_WaterWidth,
            M_PI * (2.f * m_prime - g_WaterN) / g_WaterWidth);
    real32 KLen = Length(K);
    if(KLen < 1e-6f) return 0.f;

    real32 KLen2 = Square(KLen);
    real32 KLen4 = Square(KLen2);

    vec2f UnitK = Normalize(K);
    vec2f UnitW = Normalize(g_WaterW);
    real32 KDotW = Dot(UnitK, UnitW);
    real32 KDotW2 = Square(KDotW);

    real32 WLen = Length(g_WaterW);
    real32 L = Square(WLen) / g_G;
    real32 L2 = Square(L);

    real32 Damping = 1e-3f;
    real32 DampL2 = L2 * Square(Damping);

    return g_A * (expf(-1.f / (KLen2 * L2)) / KLen4) * KDotW2 * expf(-KLen2 * DampL2);
}

complex ComputeHTilde0(int n_prime, int m_prime)
{
    complex R = GaussianRandomVariable();
    return R * sqrtf(Phillips(n_prime, m_prime) / 2.0f);
}

void WaterInitialization(game_memory *Memory, game_state *State, game_system *System)
{
    int N = (int)g_WaterN;
    int NPlus1 = N+1;

    size_t WaterAttribs = 5 * sizeof(vec3f); // Pos, Norm, hTilde0, hTilde0mk, OrigPos
    size_t WaterVertexDataSize = Square(NPlus1) * WaterAttribs;
    size_t WaterVertexCount = Square(NPlus1);
    real32 *WaterVertexData = (real32*)PushArenaData(&Memory->ScratchArena, WaterVertexDataSize);

    size_t WaterIndexDataSize = Square(N) * 6 * sizeof(uint32);
    uint32 *WaterIndexData = (uint32*)PushArenaData(&Memory->ScratchArena, WaterIndexDataSize);

    water_system *WaterSystem = (water_system*)PushArenaStruct(&Memory->ScratchArena, water_system);

    System->WaterSystem = WaterSystem;
    WaterSystem->VertexDataSize = WaterVertexDataSize;
    WaterSystem->VertexCount = WaterVertexCount;
    WaterSystem->VertexData = WaterVertexData;
    WaterSystem->IndexDataSize = WaterIndexDataSize;
    WaterSystem->IndexData = WaterIndexData;
    WaterSystem->Positions = WaterSystem->VertexData;
    WaterSystem->Normals = WaterSystem->VertexData + WaterVertexCount;
    WaterSystem->HTilde0 = WaterSystem->VertexData + 2 * WaterVertexCount;
    WaterSystem->HTilde0mk = WaterSystem->VertexData + 3 * WaterVertexCount;
    WaterSystem->OrigPositions = WaterSystem->VertexData + 4 * WaterVertexCount;

    void *ht = PushArenaData(&Memory->ScratchArena, g_WaterN*g_WaterN * sizeof(complex));
    WaterSystem->hTilde = (complex*)ht;
    WaterSystem->hTildeSlopeX = (complex*)PushArenaData(&Memory->ScratchArena, g_WaterN*g_WaterN * sizeof(complex));
    WaterSystem->hTildeSlopeZ = (complex*)PushArenaData(&Memory->ScratchArena, g_WaterN*g_WaterN * sizeof(complex));
    WaterSystem->hTildeDX = (complex*)PushArenaData(&Memory->ScratchArena, g_WaterN*g_WaterN * sizeof(complex));
    WaterSystem->hTildeDZ = (complex*)PushArenaData(&Memory->ScratchArena, g_WaterN*g_WaterN * sizeof(complex));
    
    vec3f *HTilde0Arr = (vec3f*)WaterSystem->HTilde0;
    vec3f *HTilde0mkArr = (vec3f*)WaterSystem->HTilde0mk;
    vec3f *OrigPositions = (vec3f*)WaterSystem->OrigPositions;
    vec3f *Positions = (vec3f*)WaterSystem->Positions;
    vec3f *Normals = (vec3f*)WaterSystem->Normals;
    uint32 *Indices = (uint32*)WaterSystem->IndexData;

    complex HTilde0, HTilde0mk;
    for(int m_prime = 0; m_prime < NPlus1; m_prime++)
    {
        for(int n_prime = 0; n_prime < NPlus1; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            HTilde0 = ComputeHTilde0(n_prime, m_prime);
            HTilde0mk = Conjugate(ComputeHTilde0(-n_prime, -m_prime));

            HTilde0Arr[Idx].x = HTilde0.r;
            HTilde0Arr[Idx].y = HTilde0.i;
            HTilde0mkArr[Idx].x = HTilde0mk.r;
            HTilde0mkArr[Idx].y = HTilde0mk.i;

            Positions[Idx].x = OrigPositions[Idx].x = (n_prime - N / 2.0f);
            Positions[Idx].y = OrigPositions[Idx].y = 0.f;
            Positions[Idx].z = OrigPositions[Idx].z = (n_prime - N / 2.0f);

            Normals[Idx] = vec3f(0, 1, 0);
        }
    }

    uint32 IndexCount = 0;
    for(int m_prime = 0; m_prime < N; m_prime++)
    {
        for(int n_prime = 0; n_prime < N; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            
            Indices[IndexCount++] = Idx;
            Indices[IndexCount++] = Idx + NPlus1;
            Indices[IndexCount++] = Idx + NPlus1 + 1;
            Indices[IndexCount++] = Idx;
            Indices[IndexCount++] = Idx + NPlus1 + 1;
            Indices[IndexCount++] = Idx + 1;
        }
    }
    WaterSystem->IndexCount = IndexCount;
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
    System->ConsoleLog = ConsoleLog;
    System->SoundData = SoundBuffer;

    WaterInitialization(Memory, State, System);

    FillAudioBuffer(SoundBuffer);
    SoundBuffer->ReloadSoundBuffer = true;

    State->DisableMouse = false;
    State->PlayerPosition = vec3f(300, 300, 0);

    InitCamera(&State->Camera, Memory);

    State->LightColor = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
    State->WaterCounter = 0.0;

    Memory->IsInitialized = false;
    Memory->IsGameInitialized = true;
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

void UpdateWater(game_state *State, game_system *System, game_input *Input)
{
    State->WaterCounter += Input->dTime;
    //if(State->WaterCounter >= (1.0)) State->WaterCounter -= 1.0;
    vec3f *WaterPositions = (vec3f*)System->WaterSystem->Positions;
    vec3f *WaterNormals = (vec3f*)System->WaterSystem->Normals;

    vec2f SubdivDim = g_WaterWidth / (real32)g_WaterN;
    float Start = 0.10f;
    float Speed = 0.15f;
    float Magnitude[] = { 0.7f, 0.3f };
    float Lambda[] = { 0.1f, 0.05f };
    float G = 9.8f;
    float Omega[] = { sqrtf(Magnitude[0]*G), sqrtf(Magnitude[1]*G) };
    vec2f Dir[] = { vec2f(-0.5,-0.5), vec2f(-0.6,-0.4) };
    Dir[0] = Normalize(Dir[0]);
    Dir[1] = Normalize(Dir[1]);

#if 0
    vec3f *WaterFaceNormals = (vec3f*)System->WaterSystem->FaceNormals;
    {
        // Simulation step on the positions
        for(uint32 j = 0; j < g_WaterSubdivs; ++j)
        {
            for(uint32 i = 0; i < g_WaterSubdivs; ++i)
            {
                uint32 Idx = j*g_WaterSubdivs+i;

                vec2f X_ij(i, j);
                vec2f X_ij1(i, j+1);
                //X_o = Normalize(X_o);
                vec2f X_i1j1(i+1, j+1);
                vec2f X_i1j(i+1, j);
                //X_o1 = Normalize(X_o1);
                //float ti = WaveFreq(Start, Speed, i, State->WaterCounter);
                //float ti1 = WaveFreq(Start, Speed, (i+1), State->WaterCounter);
                float tij = 0.f;
                float ti1j = 0.f;
                float tij1 = 0.f;
                float ti1j1 = 0.f;

                for(int g = 0; g < 2; ++g)
                {
                    float Ot = - Omega[g] * State->WaterCounter;
                    float Pi2L = M_PI * 2.f / Lambda[g];
                    tij += Magnitude[g] * cosf(Dot(X_ij,Dir[g])*Pi2L + Ot);
                    ti1j += Magnitude[g] * cosf(Dot(X_i1j,Dir[g])*Pi2L + Ot);
                    tij1 += Magnitude[g] * cosf(Dot(X_ij1,Dir[g])*Pi2L + Ot);
                    ti1j1 += Magnitude[g] * cosf(Dot(X_i1j1,Dir[g])*Pi2L + Ot);
                }

                WaterPositions[Idx*4+0] = vec3f(i*SubdivDim.x, tij, j*SubdivDim.y);
                WaterPositions[Idx*4+1] = vec3f(i*SubdivDim.x, tij1, (j+1)*SubdivDim.y);
                WaterPositions[Idx*4+2] = vec3f((i+1)*SubdivDim.x, ti1j1, (j+1)*SubdivDim.y);
                WaterPositions[Idx*4+3] = vec3f((i+1)*SubdivDim.x, ti1j, j*SubdivDim.y);

                WaterFaceNormals[Idx*2+0] = Cross(WaterPositions[Idx*4+1] - WaterPositions[Idx*4+0],
                        WaterPositions[Idx*4+2] - WaterPositions[Idx*4+0]);
                WaterFaceNormals[Idx*2+1] = Cross(WaterPositions[Idx*4+2] - WaterPositions[Idx*4+0],
                        WaterPositions[Idx*4+3] - WaterPositions[Idx*4+0]);
            }
        }
        // Regenerate normals
        int32 IdxOffset[] = { 0, g_WaterSubdivs, g_WaterSubdivs + 1, 1 };
        for(uint32 j = 0; j < g_WaterSubdivs; ++j)
        {
            for(uint32 i = 0; i < g_WaterSubdivs; ++i)
            {
                int32 Idx = j * g_WaterSubdivs + i;
                for(uint32 v = 0; v < 4; ++v)
                {
                    int32 FaceIdx = Idx + IdxOffset[v];
                    int32 IdxTL = FaceIdx - g_WaterSubdivs - 1;
                    int32 IdxTR = FaceIdx - g_WaterSubdivs;
                    int32 IdxBL = FaceIdx - 1;
                    int32 IdxBR = FaceIdx;

                    // Add Face normals from the 4 surrounding quads
                    vec3f Sum(0.f);

                    int32 iIdx = FaceIdx % g_WaterSubdivs;
                    int32 jIdx = FaceIdx / g_WaterSubdivs;
                    if(iIdx > 0)
                    {
                        if(jIdx > 0)
                        {
                            Sum += WaterFaceNormals[IdxTL*2+0] + WaterFaceNormals[IdxTL*2+1];
                        }
                        if(jIdx < ((int32)g_WaterSubdivs-1))
                        {
                            Sum += WaterFaceNormals[IdxBL*2+1];
                        }
                    }
                    if(iIdx < (int32)(g_WaterSubdivs-1))
                    {
                        if(jIdx > 0)
                        {
                            Sum += WaterFaceNormals[IdxTR*2+0];
                        }
                        if(jIdx < ((int32)g_WaterSubdivs-1))
                        {
                            Sum += WaterFaceNormals[IdxBR*2+0] + WaterFaceNormals[IdxBR*2+1];
                        }
                    }
                    WaterNormals[Idx*4+v] = Normalize(Sum);
                }
            }
        }
    }
#endif


}
DLLEXPORT GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsGameInitialized)
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

    UpdateWater(State, System, Input);

    if(Counter > 0.75)
    {
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        LogString(System->ConsoleLog, Msg);
        Counter = 0.0;
    }
}
