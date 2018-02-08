#include <algorithm>
#include "sun.h"
#include "SFMT.h"
#include "rf/context.h"

namespace game {
// In meters
const real32 EarthRadius = 6.3710088e6;
const real32 SunDistance = 1.496e11;

void InitCamera(camera *Camera, config *Config)
{
    Camera->Position = Config->CameraPosition;
    Camera->Target = Config->CameraTarget;
    Camera->Up = vec3f(0,1,0);
    Camera->Forward = Normalize(Camera->Target - Camera->Position);
    Camera->Right = Normalize(Cross(Camera->Forward, Camera->Up));
    Camera->Up = Normalize(Cross(Camera->Right, Camera->Forward));
    Camera->LinearSpeed = Config->CameraSpeedBase;
    Camera->AngularSpeed = Config->CameraSpeedAngular;
    Camera->SpeedMult = Config->CameraSpeedMult;
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

bool Init(state *State, config *Config)
{
    State->EngineTime = 0.0;
    State->Counter = 0.1;       // init those past their reset time to not have to wait the reset time at game start 
    State->CounterTenth = 0.1;
    State->PlayerPosition = vec3f(300, 300, 0);
    State->DisableMouse.Switch(); // Set On

    InitCamera(&State->Camera, Config);

	State->Latitude = 5.345317f * DEG2RAD;
	State->EarthTilt = 23.43f * DEG2RAD;
    State->SunSpeed = Config->TimeScale * (M_PI / 86400.f);
    State->SunDirection = SphericalToCartesian(0.46 * M_PI, M_TWO_PI * 0.37);
    State->LightColor = vec4f(1.0f, 0.6, 0.5, 1.0f);
    State->WaterCounter = 0.0;
    State->WaterStateInterp = 0.f;
    State->WaterState = 1;
    State->WaterDirection = 0.f;

    return true;
}

static void UpdateUI(state *State, rf::input *Input, rf::context *Context)
{
    int32 TextlineCount = 4;
    if(State->CounterTenth >= 0.1)
    {
        const camera &Camera = State->Camera;
        snprintf(State->Textline[0].String, UI_STRINGLEN, "Camera : From <%.1f, %.1f, %.1f> To <%.1f, %.1f, %.1f>",
                Camera.Position.x, Camera.Position.y, Camera.Position.z, Camera.Target.x, Camera.Target.y, Camera.Target.z);

        snprintf(State->Textline[1].String, UI_STRINGLEN, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        State->CounterTenth = 0.0;
    }

    if(State->Counter >= 0.75)
    {
        State->Textline[0].Position = vec2i(4, 4);
        State->Textline[0].Color = rf::ui::COLOR_DEBUGFG;

        State->Textline[1].Position = vec2i(4, 18);
        State->Textline[1].Color = rf::ui::COLOR_DEBUGFG;

        snprintf(State->Textline[2].String, UI_STRINGLEN, "Water State : %d  Water Interpolant : %g", State->WaterState, State->WaterStateInterp);
        State->Textline[2].Position = vec2i(4, 32);
        State->Textline[2].Color = rf::ui::COLOR_DEBUGFG;

        State->Textline[3].Color = rf::ui::COLOR_DEBUGFG;
        State->Textline[3].Position = vec2i(4,46);

        int CharWritten = snprintf(State->Textline[3].String, UI_STRINGLEN, "Sun Speed : %.3f", State->SunSpeed);

        if(State->IsNight)
            snprintf(State->Textline[3].String + CharWritten, UI_STRINGLEN - CharWritten, " Day");
        else
            snprintf(State->Textline[3].String + CharWritten, UI_STRINGLEN - CharWritten, " Night");

        State->Counter = 0.0;
    }


    int32 LineGap = rf::ui::GetFontLineGap(rf::ui::FONT_DEFAULT);

    int UIStackHeight = LineGap * State->TextlineCount;
    static uint32 UIStackPanel = 0;
    static vec3i  UIStackPanelPos(0,0,0);
    static vec2i  UIStackPanelSize(360, UIStackHeight + 30);
    rf::ui::BeginPanel(&UIStackPanel, "", &UIStackPanelPos, &UIStackPanelSize, rf::ui::COLOR_PANELBG, rf::ui::DECORATION_NONE);
        for(uint32 i = 0; i < State->TextlineCount; ++i)
        {
            rf::ui::text_line *Line = &State->Textline[i];
            rf::ui::MakeText((void*)Line, Line->String, rf::ui::FONT_DEFAULT, Line->Position, Line->Color, 1.f, Context->WindowWidth);
        }
    rf::ui::EndPanel();
}

void MovePlayer(state *State, rf::input *Input, rf::context *Context)
{
    camera &Camera = State->Camera;
    vec2i MousePos = vec2i(Input->MousePosX, Input->MousePosY);

    vec3f CameraMove(0, 0, 0);
    if(KEY_DOWN(Input->Keys[KEY_W])) CameraMove += Camera.Forward;
    if(KEY_DOWN(Input->Keys[KEY_S])) CameraMove -= Camera.Forward;
    if(KEY_DOWN(Input->Keys[KEY_A])) CameraMove -= Camera.Right;
    if(KEY_DOWN(Input->Keys[KEY_D])) CameraMove += Camera.Right;
    if(KEY_DOWN(Input->Keys[KEY_R])) CameraMove += Camera.Up;
    if(KEY_DOWN(Input->Keys[KEY_F])) CameraMove -= Camera.Up;

    if(KEY_DOWN(Input->Keys[KEY_LEFT_SHIFT]))      Camera.SpeedMode = 1;
    else if(KEY_DOWN(Input->Keys[KEY_LEFT_CONTROL]))  Camera.SpeedMode = -1;
    else                                Camera.SpeedMode = 0;

    Normalize(CameraMove);
    real32 SpeedMult = Camera.SpeedMode ? (Camera.SpeedMode > 0 ? Camera.SpeedMult : 1.0f / Camera.SpeedMult) : 1.0f;
    CameraMove *= (real32)(Input->dTime * Camera.LinearSpeed * SpeedMult);
    Camera.Position += CameraMove;

    Camera.Position.y = Max(0.5, Camera.Position.y); // Min at .5 meters

    if(MOUSE_HIT(Input->MouseRight))
    {
        Camera.FreeflyMode = true;
        State->DisableMouse.Switch();
        Camera.LastMousePos = MousePos;
    }
    if(MOUSE_UP(Input->MouseRight))
    {
        Camera.FreeflyMode = false;
        State->DisableMouse.Switch();
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
    Camera.ViewMatrix = mat4f::LookAt(Camera.Position, Camera.Target, Camera.Up);

    vec3f Move;
    Move.x = (real32)Input->MousePosX;
    Move.y = (real32)(540-Input->MousePosY);

    if(Move.x < 0) Move.x = 0;
    if(Move.y < 0) Move.y = 0;
    if(Move.x >= 960) Move.x = 959;
    if(Move.y >= 540) Move.y = 539;

    State->PlayerPosition = Move;

    // Mouse cursor disabling when in Freefly
    if(State->DisableMouse.Switched())
    {
        if(State->DisableMouse)
        {
            rf::ctx::ShowCursor(Context, false);
        }
        else
        {
            rf::ctx::ShowCursor(Context, true);
        }

        State->DisableMouse.Update();
    }
}

void UpdateSky(game::state *State, rf::input *Input)
{
	State->DayPhase = fmod(State->DayPhase + State->SunSpeed * M_PI * Input->dTime, 2.f * M_PI);


	real32 CosET = cosf(State->EarthTilt);
	
	vec3f SunPos(SunDistance * CosET * cosf(State->DayPhase) - EarthRadius * cosf(State->Latitude),
				 SunDistance * sinf(State->EarthTilt) - EarthRadius * sinf(State->Latitude),
				 SunDistance * CosET * sinf(State->DayPhase));
	mat4f Rot;
	Rot = Rot.RotateZ(M_PI_OVER_TWO - State->Latitude);
	SunPos = Rot * SunPos;
	
	//snprintf(Msg, CONSOLE_STRINGLEN, "%e %e %e", Rot[1][1], SunPos.y, SunPos.z); 
	//LogString(System->ConsoleLog, Msg);

    if(State->IsNight && SunPos.y > 0.f) {
        State->IsNight = false;
    }
    if(!State->IsNight && SunPos.y < 0.f) {
        State->IsNight = true;
    }

	State->SunDirection = Normalize(SunPos);
    //State->LightColor = vec4f(0.9f, 0.7, 0.8, 1.0f);
    State->LightColor = vec4f(2.7f, 2.1, 2.4, 1.0f);
}

void Update(state *State, rf::input *Input, rf::context *Context)
{
    State->Counter += Input->dTime; 
    State->CounterTenth += Input->dTime; 

    UpdateUI(State, Input, Context);

    MovePlayer(State, Input, Context);

    // Water state
#if 0
    if(KEY_DOWN(Input->Keys[KEY_KP_ADD]))
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

    if(KEY_DOWN(Input->Keys[KEY_KP_SUBTRACT]))
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
#endif

    // Sun state
    if(KEY_DOWN(Input->Keys[KEY_KP_MULTIPLY]))
    {
       // State->WaterDirection += Input->dTime * 0.05;
        State->SunSpeed += 0.0005;
    }

    if(KEY_DOWN(Input->Keys[KEY_KP_DIVIDE]))
    {
        //State->WaterDirection -= Input->dTime * 0.05;
        State->SunSpeed -= 0.0005;
    }

    if(KEY_HIT(Input->Keys[KEY_P]))
        State->SunSpeed = 0.f;

    UpdateSky(State, Input);
}

void Destroy(state *State)
{
}

}

#if 0

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
    FillAudioBuffer(System->SoundData);
    System->SoundData->ReloadSoundBuffer = true;
}



DLLEXPORT GAMEUPDATE(GameUpdate)
{

#if 0
    if(Input->KeyReleased)
    {
        tmp_sound_data *SoundData = System->SoundData;
        FillAudioBuffer(SoundData);
        SoundData->ReloadSoundBuffer = true;
    }
#endif


    UpdateSky(Local, State, System, Input);
}
#endif
