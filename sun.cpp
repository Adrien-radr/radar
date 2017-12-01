#include <algorithm>
#include "sun.h"
#include "SFMT.h"

// In meters
const real32 EarthRadius = 6.3710088e6;
const real32 SunDistance = 1.496e11;

// NOTE - Storage for game data local to the Sun DLL
// This is pushed on the Session stack of the engine
struct sun_storage
{
    real64 Counter;
    real64 CounterTenth;
    bool IsNight;
	// DayPhase: 0 = noon, pi = midnight
	real32 DayPhase;
	// Varies with season
	real32 EarthTilt;
	// Latitude: -pi/2 = South pole, +pi/2 = North pole
	real32 Latitude;

    ui::text_line FPSText;
    ui::text_line NightDayText;
    ui::text_line WaterText;
    ui::text_line CameraText;
};

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
    Local->CounterTenth = 0.0;

    FillAudioBuffer(System->SoundData);
    System->SoundData->ReloadSoundBuffer = true;

    State->DisableMouse = false;
    State->PlayerPosition = vec3f(300, 300, 0);

    InitCamera(&State->Camera, Memory);
	
	// Bretagne, France
	Local->Latitude = 48.2020f * DEG2RAD;
	// Abidjan, Ivory Coast
	//Local->Latitude = 5.345317f * DEG2RAD;
	// Summer solstice
	Local->EarthTilt = 23.43f * DEG2RAD;

    // TODO - Pack Sun color and direction from envmaps
#if 1
    // Monument Envmap
    State->LightDirection = SphericalToCartesian(0.46 * M_PI, M_TWO_PI * 0.37);
    State->LightColor = vec4f(1.0f, 0.6, 0.5, 1.0f);
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

    if(KEY_DOWN(Input->KeyLShift))      Camera.SpeedMode = 1;
    else if(KEY_DOWN(Input->KeyLCtrl))  Camera.SpeedMode = -1;
    else                                Camera.SpeedMode = 0;

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

            Camera.Theta = std::max(1e-5f, std::min(real32(M_PI) - 1e-5f, Camera.Theta));
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
    float Sunspeed = 0.05f;
	Local->DayPhase = fmod(Local->DayPhase + Sunspeed * M_PI * Input->dTime, 2.f * M_PI);


	real32 CosET = cosf(Local->EarthTilt);
	
	vec3f SunPos(SunDistance * CosET * cosf(Local->DayPhase) - EarthRadius * cosf(Local->Latitude),
				 SunDistance * sinf(Local->EarthTilt) - EarthRadius * sinf(Local->Latitude),
				 SunDistance * CosET * sinf(Local->DayPhase));
	mat4f Rot;
	Rot = Rot.RotateZ(M_PI_OVER_TWO - Local->Latitude);
	SunPos = Rot * SunPos;
	
	//snprintf(Msg, CONSOLE_STRINGLEN, "%e %e %e", Rot[1][1], SunPos.y, SunPos.z); 
	//LogString(System->ConsoleLog, Msg);

    if(Local->IsNight && SunPos.y > 0.f) {
        Local->IsNight = false;
    }
    if(!Local->IsNight && SunPos.y < 0.f) {
        Local->IsNight = true;
    }

	State->LightDirection = Normalize(SunPos);
    //State->LightColor = vec4f(0.9f, 0.7, 0.8, 1.0f);
    State->LightColor = vec4f(2.7f, 2.1, 2.4, 1.0f);
}

void MakeUIText(ui::frame_stack *Stack, ui::text_line *Text)
{
    Stack->TextLines[Stack->TextLineCount++] = Text;
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
    ui::frame_stack *UIStack = System->UIStack;

#if 0
    if(Input->KeyReleased)
    {
        tmp_sound_data *SoundData = System->SoundData;
        FillAudioBuffer(SoundData);
        SoundData->ReloadSoundBuffer = true;
    }
#endif
    
    Local->Counter += Input->dTime; 
    Local->CounterTenth += Input->dTime; 

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
            State->WaterStateInterp = std::min(1.f, State->WaterStateInterp);
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
            State->WaterStateInterp = std::max(0.f, State->WaterStateInterp);
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

    if(KEY_HIT(Input->KeyF3))
    {
        //rlog::Msg("Test Console text");
    }

    UpdateSky(Local, State, System, Input);

    if(Local->CounterTenth > 0.1)
    {
        const game_camera &Camera = State->Camera;
        snprintf(Local->CameraText.String, UI_STRINGLEN, "Camera : From <%.1f, %.1f, %.1f> To <%.1f, %.1f, %.1f>",
                Camera.Position.x, Camera.Position.y, Camera.Position.z, Camera.Target.x, Camera.Target.y, Camera.Target.z);

        snprintf(Local->FPSText.String, UI_STRINGLEN, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Local->CounterTenth = 0.0;
    }

    if(Local->Counter > 0.75)
    {
        Local->CameraText.Position = vec2i(4, 4);
        Local->CameraText.Color = ui::COLOR_DEBUGFG;

        Local->FPSText.Position = vec2i(4, 18);
        Local->FPSText.Color = ui::COLOR_DEBUGFG;

        snprintf(Local->WaterText.String, UI_STRINGLEN, "Water State : %d  Water Interpolant : %g", State->WaterState, State->WaterStateInterp);
        Local->WaterText.Position = vec2i(4, 32);
        Local->WaterText.Color = ui::COLOR_DEBUGFG;

        Local->NightDayText.Color = ui::COLOR_DEBUGFG;
        Local->NightDayText.Position = vec2i(4,46);
        if(Local->IsNight)
            snprintf(Local->NightDayText.String, UI_STRINGLEN, "Day");
        else
            snprintf(Local->NightDayText.String, UI_STRINGLEN, "Night");

        Local->Counter = 0.0;
    }

    // TODO - streamline this a bit
    MakeUIText(UIStack, &Local->FPSText);
    MakeUIText(UIStack, &Local->CameraText);
    MakeUIText(UIStack, &Local->WaterText);
    MakeUIText(UIStack, &Local->NightDayText);
}
