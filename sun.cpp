#include "sun.h"
#include <stdio.h>
#include <string.h>
#include <math.h>


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
    float U, V, W;
    do {
        U = 2.f * rand()/(real32)RAND_MAX - 1.f;
        V = 2.f * rand()/(real32)RAND_MAX - 1.f;
        W = Square(U) + Square(V);
    } while(W >= 1.f);
    W = sqrtf((-2.f * logf(W)) / W);
    return complex(U * W, V * W);
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

real32 ComputeDispersion(int n_prime, int m_prime)
{
    real32 W0 = 2.f * M_PI / 200.f;
    real32 Kx = M_PI * (2 * n_prime - g_WaterN) / g_WaterWidth;
    real32 Kz = M_PI * (2 * m_prime - g_WaterN) / g_WaterWidth;
    return floorf(sqrtf(g_G * sqrtf(Square(Kx) + Square(Kz))) / W0) * W0;
}

complex ComputeHTilde0(int n_prime, int m_prime)
{
    complex R = GaussianRandomVariable();
    return R * sqrtf(Phillips(n_prime, m_prime) / 2.0f);
}

complex ComputeHTilde(water_system *WaterSystem, real32 T, int n_prime, int m_prime)
{
    int NPlus1 = g_WaterN+1;
    int Idx = m_prime * NPlus1 + n_prime;

    vec3f *HTilde0 = (vec3f*)WaterSystem->HTilde0;
    vec3f *HTilde0mk = (vec3f*)WaterSystem->HTilde0mk;
    
    complex H0(HTilde0[Idx].x, HTilde0[Idx].y);
    complex H0mk(HTilde0mk[Idx].x, HTilde0mk[Idx].y);

    real32 OmegaT = ComputeDispersion(n_prime, m_prime) * T;
    real32 CosOT = cosf(OmegaT);
    real32 SinOT = sinf(OmegaT);

    complex C0(CosOT, SinOT);
    complex C1(CosOT, -SinOT);

    return H0 * C0 + H0mk * C1;
}

struct wave_vector
{
    complex H; // Wave Height
    vec2f   D; // Wave XZ Displacement
    vec3f   N; // Wave Normal
};

wave_vector ComputeWave(water_system *WaterSystem, vec2f X, real32 T)
{
    int N = g_WaterN;
    int NPlus1 = N+1;

    wave_vector V = {};

    complex C, Res, HTildeC;
    vec2f K;
    real32 Kx, Kz, KLen, KDotX;

    for(int m_prime = 0; m_prime < N; ++m_prime)
    {
        Kz = 2.f * M_PI * (m_prime - N / 2.f) / g_WaterWidth;
        for(int n_prime = 0; n_prime < N; ++n_prime)
        {
            Kx = 2.f * M_PI * (n_prime - N / 2.f) / g_WaterWidth;
            K = vec2f(Kx, Kz);
            KLen = Length(K);
            KDotX = Dot(K, X);

            C = complex(cosf(KDotX), sinf(KDotX));
            HTildeC = ComputeHTilde(WaterSystem, T, n_prime, m_prime) * C;

            V.H = V.H + HTildeC;
            V.N = V.N + vec3f(-Kx * HTildeC.i*g_WaterWidth, 0.f, -Kz * HTildeC.i*g_WaterWidth);
            if(KLen < 1e-6f) continue;
            V.D = V.D + vec2f(Kx / KLen * HTildeC.i, Kz / KLen * HTildeC.i);
        }
    }

    //V.N = Normalize(vec3f(0, 1, 0) - V.N);
    V.N = vec3f(0, 1, 0) - Normalize(V.N);
    //V.N = Normalize(V.N);

    return V;
}

void UpdateWater(game_state *State, game_system *System, game_input *Input)
{
    State->WaterCounter += Input->dTimeFixed;

    vec3f *WaterPositions = (vec3f*)System->WaterSystem->Positions;
    vec3f *WaterHTilde0 = (vec3f*)System->WaterSystem->HTilde0;
    vec3f *WaterNormals = (vec3f*)System->WaterSystem->Normals;
    vec3f *WaterOrigPositions = (vec3f*)System->WaterSystem->OrigPositions;

    int N = g_WaterN;
    int NPlus1 = N+1;

    float Lambda = -1.f;
    vec2f X;
    vec2f D;
    wave_vector Wave;

    for(int m_prime = 0; m_prime < N; ++m_prime)
    {
        for(int n_prime = 0; n_prime < N; ++n_prime)
        {
            int Idx = m_prime * NPlus1 + n_prime;

            X = vec2f(WaterPositions[Idx].x, WaterPositions[Idx].z);
            Wave = ComputeWave(System->WaterSystem, X, 2*(real32)State->WaterCounter);

            WaterPositions[Idx].y = Wave.H.r;
            WaterPositions[Idx].x = WaterOrigPositions[Idx].x + Lambda * Wave.D.x;
            WaterPositions[Idx].z = WaterOrigPositions[Idx].z + Lambda * Wave.D.y;

            WaterNormals[Idx] = Wave.N;

            // NOTE - Fill in the far side to finish our quads
            if(n_prime == 0 && m_prime == 0)
            {
                WaterPositions[Idx + N + NPlus1 * N].y = Wave.H.r;
                WaterPositions[Idx + N + NPlus1 * N].x = WaterOrigPositions[Idx + N + NPlus1 * N].x + Lambda * Wave.D.x;
                WaterPositions[Idx + N + NPlus1 * N].z = WaterOrigPositions[Idx + N + NPlus1 * N].z + Lambda * Wave.D.y;

                WaterNormals[Idx + N + NPlus1 * N] = Wave.N;
            }
            if(n_prime == 0)
            {
                WaterPositions[Idx + N].y = Wave.H.r;
                WaterPositions[Idx + N].x = WaterOrigPositions[Idx + N].x + Lambda * Wave.D.x;
                WaterPositions[Idx + N].z = WaterOrigPositions[Idx + N].z + Lambda * Wave.D.y;

                WaterNormals[Idx + N] = Wave.N;
            }
            if(m_prime == 0)
            {
                WaterPositions[Idx + NPlus1 * N].y = Wave.H.r;
                WaterPositions[Idx + NPlus1 * N].x = WaterOrigPositions[Idx + NPlus1 * N].x + Lambda * Wave.D.x;
                WaterPositions[Idx + NPlus1 * N].z = WaterOrigPositions[Idx + NPlus1 * N].z + Lambda * Wave.D.y;

                WaterNormals[Idx + NPlus1 * N] = Wave.N;
            }
        }
    }
}

void WaterInitialization(game_memory *Memory, game_state *State, game_system *System)
{
    int N = g_WaterN;
    int NPlus1 = N+1;

    size_t WaterAttribs = 5 * sizeof(vec3f); // Pos, Norm, hTilde0, hTilde0mk, OrigPos
    size_t WaterVertexDataSize = Square(NPlus1) * WaterAttribs;
    size_t WaterVertexCount = 3 * Square(NPlus1); // 3 floats per attrib
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

    void *ht = PushArenaData(&Memory->ScratchArena, N * N * sizeof(complex));
    WaterSystem->hTilde = (complex*)ht;
    WaterSystem->hTildeSlopeX = (complex*)PushArenaData(&Memory->ScratchArena, N * N * sizeof(complex));
    WaterSystem->hTildeSlopeZ = (complex*)PushArenaData(&Memory->ScratchArena, N * N * sizeof(complex));
    WaterSystem->hTildeDX = (complex*)PushArenaData(&Memory->ScratchArena, N * N * sizeof(complex));
    WaterSystem->hTildeDZ = (complex*)PushArenaData(&Memory->ScratchArena, N * N * sizeof(complex));
    
    vec3f *HTilde0 = (vec3f*)WaterSystem->HTilde0;
    vec3f *HTilde0mk = (vec3f*)WaterSystem->HTilde0mk;
    vec3f *OrigPositions = (vec3f*)WaterSystem->OrigPositions;
    vec3f *Positions = (vec3f*)WaterSystem->Positions;
    vec3f *Normals = (vec3f*)WaterSystem->Normals;
    uint32 *Indices = (uint32*)WaterSystem->IndexData;

    for(int m_prime = 0; m_prime < NPlus1; m_prime++)
    {
        for(int n_prime = 0; n_prime < NPlus1; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            complex H0 = ComputeHTilde0(n_prime, m_prime);
            complex H0mk = Conjugate(ComputeHTilde0(-n_prime, -m_prime));

            HTilde0[Idx].x = H0.r;
            HTilde0[Idx].y = H0.i;
            HTilde0mk[Idx].x = H0mk.r;
            HTilde0mk[Idx].y = H0mk.i;

            Positions[Idx].x = OrigPositions[Idx].x = (n_prime - N / 2.0f) * g_WaterWidth / N;
            Positions[Idx].y = OrigPositions[Idx].y = 0.f;
            Positions[Idx].z = OrigPositions[Idx].z = (m_prime - N / 2.0f) * g_WaterWidth / N;

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
    CameraMove *= (real32)(Input->dTimeFixed * Camera.LinearSpeed * SpeedMult);
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
            Camera.Phi += MouseOffset.x * Input->dTimeFixed * Camera.AngularSpeed;
            Camera.Theta -= MouseOffset.y * Input->dTimeFixed * Camera.AngularSpeed;

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

real64 Counter = 0.0;
real64 InputCounter = 0.0;
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

    State->LightColor = vec4f(1.0f, 1.0f, 1.0f, 1.0f);

    Counter += Input->dTime;
    InputCounter += Input->dTime;

    if(InputCounter >= Input->dTimeFixed)
    {
        InputCounter -= Input->dTimeFixed;
        MovePlayer(State, Input);
        UpdateWater(State, System, Input);
    }

    if(Counter > 0.75)
    {
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        LogString(System->ConsoleLog, Msg);
        Counter = 0.0;
    }
}
