#include "sun.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "SFMT.h"

// NOTE - Storage for game data local to the Sun DLL
// This is pushed on the Session stack of the engine
struct sun_storage
{
    real64 Counter;
    vec2f SunDir;
    bool IsNight;
};

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

    vec2f Spherical = CartesianToSpherical(Camera->Forward);
    Camera->Theta = Spherical.x;
    Camera->Phi = Spherical.y;
#if 0
    vec2f Azimuth = Normalize(vec2f(Camera->Forward[0], Camera->Forward[2]));
    Camera->Phi = atan2f(Azimuth.y, Azimuth.x);
    Camera->Theta = atan2f(Camera->Forward.y, sqrtf(Dot(Azimuth, Azimuth)));
#endif
}



void GameInitialization(game_memory *Memory)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    System->DLLStorage = PushArenaStruct(&Memory->SessionArena, sun_storage);
    sun_storage *Local = (sun_storage*)System->DLLStorage;
    Local->Counter = 0.0;

    FillAudioBuffer(System->SoundData);
    System->SoundData->ReloadSoundBuffer = true;

    State->DisableMouse = false;
    State->PlayerPosition = vec3f(300, 300, 0);

    InitCamera(&State->Camera, Memory);

    // TODO - Pack Sun color and direction from envmaps
#if 1
    // Monument Envmap
    State->LightDirection = SphericalToCartesian(0.46 * M_PI, M_TWO_PI * 0.37);
    State->LightColor = vec4f(1.0f, 0.6, 0.2, 1.0f);
    Local->SunDir = CartesianToSpherical(State->LightDirection);
    Local->IsNight = Local->SunDir.x >= M_PI_OVER_TWO;
#endif
#if 0
    // Arch Envmap
    State->LightColor = vec4f(1, 241.f/255.f, 234.f/255.f, 1.0f);
    State->LightDirection = SphericalToCartesian(0.365 * M_PI, M_TWO_PI * 0.080);
#endif
#if 0
    // Malibu Envmap
    State->LightColor = vec4f(255.f/255.f, 251.f/255.f, 232.f/255.f, 1.0f);
    State->LightDirection = SphericalToCartesian(0.15 * M_PI, M_TWO_PI * 0.95);
#endif
#if 0
    // Tropical Envmap
    State->LightColor = vec4f(255.f/255.f, 252.f/255.f, 245.f/255.f, 1.0f);
    State->LightDirection = SphericalToCartesian(0.12 * M_PI, M_TWO_PI * 0.33);
#endif
#if 0 // Skybox 1
        State->LightDirection = Normalize(vec3f(0.5, 0.2, 1.0));
#endif
#if 0 // Skybox 2
        State->LightDirection = Normalize(vec3f(0.7, 1.2, -0.7));
#endif

    State->WaterCounter = 0.0;
    State->WaterStateInterp = 0.f;
    State->WaterState = 1;
    State->WaterDirection = 0.f;

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
    if(KEY_DOWN(Input->KeyR)) CameraMove += Camera.Up;
    if(KEY_DOWN(Input->KeyF)) CameraMove -= Camera.Up;

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
            Camera.Theta += MouseOffset.y * Input->dTime * Camera.AngularSpeed;

            if(Camera.Phi > M_TWO_PI) Camera.Phi -= M_TWO_PI;
            if(Camera.Phi < 0.0f) Camera.Phi += M_TWO_PI;

            Camera.Theta = Max(1e-5f, Min(M_PI - 1e-5f, Camera.Theta));
            Camera.Forward = SphericalToCartesian(Camera.Theta, Camera.Phi);

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

void UpdateSky(sun_storage *Local, game_state *State, game_system *System, game_input *Input)
{
    Local->SunDir.x = fmod(Local->SunDir.x + 0.2f * M_PI * Input->dTime, 2.f * M_PI);

    real32 Theta, Phi;
    {
        real32 ThetaAscend = Min(M_PI, Local->SunDir.x);
        real32 ThetaDescend = Max(0.0, Local->SunDir.x - M_PI);

        if(ThetaDescend > 0.0)
        {
            Theta = M_PI - ThetaDescend;
            Phi = Local->SunDir.y - M_PI;
        }
        else
        {
            Theta = ThetaAscend;
            Phi = Local->SunDir.y;
        }
    }

    // TODO - This should just be 
    // Local->IsNight = Theta <= M_PI_OVER_TWO;
    // But temporarily added logging of the state change for testing
    if(Local->IsNight && Theta < M_PI_OVER_TWO) {
        Local->IsNight = false;
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "Day");
        LogString(System->ConsoleLog, Msg);
    }
    if(!Local->IsNight && Theta >= M_PI_OVER_TWO)
    {
        Local->IsNight = true;
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "Night");
        LogString(System->ConsoleLog, Msg);
    }

    State->LightDirection = SphericalToCartesian(Theta, Phi);
}

DLLEXPORT GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsGameInitialized)
    {
        GameInitialization(Memory);
    }

    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);
    sun_storage *Local = (sun_storage*)System->DLLStorage;

#if 0
    if(Input->KeyReleased)
    {
        tmp_sound_data *SoundData = System->SoundData;
        FillAudioBuffer(SoundData);
        SoundData->ReloadSoundBuffer = true;
    }
#endif
    
    Local->Counter += Input->dTime; 

    MovePlayer(State, Input);

    if(KEY_DOWN(Input->KeyNumPlus))
    {
        State->WaterStateInterp = State->WaterStateInterp + 0.01;

        if(State->WaterState < (water_system::BeaufortStateCount - 2))
        {
            if(State->WaterStateInterp >= 1.f)
            {
                State->WaterStateInterp -= 1.f;
                ++State->WaterState;
            }
        }
        else
        {
            State->WaterStateInterp = Min(1.f, State->WaterStateInterp);
        }
    }

    if(KEY_DOWN(Input->KeyNumMinus))
    {
        State->WaterStateInterp = State->WaterStateInterp - 0.01;

        if(State->WaterState > 0)
        {
            if(State->WaterStateInterp < 0.f)
            {
                State->WaterStateInterp += 1.f;
                --State->WaterState;
            }
        }
        else
        {
            State->WaterStateInterp = Max(0.f, State->WaterStateInterp);
        }
    }

    if(KEY_DOWN(Input->KeyNumMultiply))
    {
        State->WaterDirection += Input->dTime * 0.05;
    }

    if(KEY_DOWN(Input->KeyNumDivide))
    {
        State->WaterDirection -= Input->dTime * 0.05;
    }

    UpdateSky(Local, State, System, Input);

    if(Local->Counter > 0.75)
    {
        console_log_string Msg;
        snprintf(Msg, ConsoleLogStringLen, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        LogString(System->ConsoleLog, Msg);
        Local->Counter = 0.0;
    }
}
