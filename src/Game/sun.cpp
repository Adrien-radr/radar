#include <algorithm>
#include "sun.h"
#include "rf/context.h"

static vec3f CameraDefaultPos(0, 0, 0);

namespace game {
// In meters
const real32 EarthRadius = 6.3710088e6f;
const real32 SunDistance = 1.496e11f;

void InitCamera(camera *Camera, config *Config)
{
	Camera->Position = Config->CameraPosition;
	CameraDefaultPos = Config->CameraPosition;
	Camera->PositionDecimal = vec3f(0, 0, 0);
	Camera->Forward = Config->CameraForward;
	Camera->Up = vec3f(0, 1, 0);
	Camera->Right = Normalize(Cross(Camera->Forward, Camera->Up));
	Camera->Up = Normalize(Cross(Camera->Right, Camera->Forward));
	//Camera->Target = Camera->Position + Camera->Forward;
	Camera->LinearSpeed = Config->CameraSpeedBase;
	Camera->AngularSpeed = Config->CameraSpeedAngular;
	for (int i = 0; i < 4; ++i)
	{
		Camera->SpeedMult[i] = Config->CameraSpeedMult[i];
	}
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
	State->TextLineCount = 0;
	State->EngineTime = 0.0;
	State->Counter = 0.1;       // init those past their reset time to not have to wait the reset time at game start 
	State->CounterTenth = 0.1;
	State->DisableMouse.Switch(); // Set On

	InitCamera(&State->Camera, Config);

	State->Latitude = 5.345317f * DEG2RAD;
	State->EarthTilt = 23.43f * DEG2RAD;
	State->SunSpeed = Config->TimeScale * (M_PI / 86400.f);
	State->SunDirection = SphericalToCartesian(0.46f * M_PI, M_TWO_PI * 0.37f);
	State->LightColor = vec4f(1.0f, 0.6f, 0.5f, 1.0f);
	State->WaterCounter = 0.0;
	State->WaterStateInterp = 0.f;
	State->WaterState = 1;
	State->WaterDirection = 0.f;

	return true;
}

static void UpdateUI(state *State, rf::input *Input, rf::context *Context)
{
	//int32 TextlineCount = 4;
	State->TextLineCount = 5; // 0 : Camera Pos, 1, Camera movement, 2 : FPS/Mouse, 3 : Water, 4 : Atmosphere

	if (State->CounterTenth >= 0.1)
	{
		const camera &Camera = State->Camera;
		State->Textline[0].Position = vec2i(4, 4);
		State->Textline[0].Color = rf::ui::COLOR_DEBUGFG;

		snprintf(State->Textline[0].String, UI_STRINGLEN, "Camera : <%.0f, %.0f, %.0f> <%.4f, %.4f, %.4f> <%.2f, %.2f>",
			Camera.Position.x, Camera.Position.y, Camera.Position.z, 
			Camera.PositionDecimal.x, Camera.PositionDecimal.y, Camera.PositionDecimal.z, 
			Camera.Theta, Camera.Phi);

		State->Textline[1].Position = vec2i(4, 18);
		State->Textline[1].Color = rf::ui::COLOR_DEBUGFG;

		State->Textline[2].Position = vec2i(4, 32);
		State->Textline[2].Color = rf::ui::COLOR_DEBUGFG;

		snprintf(State->Textline[2].String, UI_STRINGLEN, "%2.4g, Mouse: %d,%d", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
		
		State->CounterTenth = 0.0;
	}

	if (State->Counter >= 0.75)
	{
		State->Textline[3].Position = vec2i(4, 46);
		State->Textline[3].Color = rf::ui::COLOR_DEBUGFG;

		snprintf(State->Textline[3].String, UI_STRINGLEN, "Water State : %d  Water Interpolant : %g", State->WaterState, State->WaterStateInterp);

		State->Textline[4].Color = rf::ui::COLOR_DEBUGFG;
		State->Textline[4].Position = vec2i(4, 60);

		int CharWritten = snprintf(State->Textline[4].String, UI_STRINGLEN, "Sun Speed : %.3f", State->SunSpeed);

		if (State->IsNight)
			snprintf(State->Textline[4].String + CharWritten, UI_STRINGLEN - CharWritten, " Day");
		else
			snprintf(State->Textline[4].String + CharWritten, UI_STRINGLEN - CharWritten, " Night");

		State->Counter = 0.0;
	}


	int32 LineGap = rf::ui::GetFontLineGap(rf::ui::FONT_DEFAULT);

	int UIStackHeight = LineGap * State->TextLineCount;
	static uint32 UIStackPanel = 0;
	static vec3i  UIStackPanelPos(0, 0, 0);
	static vec2i  UIStackPanelSize(400, UIStackHeight + 20);
	rf::ui::BeginPanel(&UIStackPanel, "", &UIStackPanelPos, &UIStackPanelSize, rf::ui::COLOR_PANELBG, rf::ui::DECORATION_NONE);
	for (int32 i = 0; i < State->TextLineCount; ++i)
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

	// Camera reset pos
	if (KEY_RELEASED(Input->Keys[KEY_Z]) && KEY_PRESSED(Input->Keys[KEY_LEFT_CONTROL]))
	{
		Camera.Position = CameraDefaultPos;
		Camera.PositionDecimal = vec3f(0);
	}

	vec3f CameraMove(0, 0, 0);
	if (KEY_PRESSED(Input->Keys[KEY_W])) CameraMove += Camera.Forward;
	if (KEY_PRESSED(Input->Keys[KEY_S])) CameraMove -= Camera.Forward;
	if (KEY_PRESSED(Input->Keys[KEY_A])) CameraMove -= Camera.Right;
	if (KEY_PRESSED(Input->Keys[KEY_D])) CameraMove += Camera.Right;
	if (KEY_PRESSED(Input->Keys[KEY_R])) CameraMove += Camera.Up;
	if (KEY_PRESSED(Input->Keys[KEY_F])) CameraMove -= Camera.Up;

	if (KEY_PRESSED(Input->Keys[KEY_LEFT_SHIFT]))
	{
		if (KEY_PRESSED(Input->Keys[KEY_LEFT_CONTROL]))
		{ // Super Speed
			Camera.SpeedMode = 3;
		}
		else
		{
			Camera.SpeedMode = 2;
		}
	}
	else if (KEY_PRESSED(Input->Keys[KEY_LEFT_CONTROL]))
	{
		Camera.SpeedMode = 0;
	}
	else
	{
		Camera.SpeedMode = 1;
	}

	Normalize(CameraMove);
	real32 SpeedMult = Camera.SpeedMult[Camera.SpeedMode];
	CameraMove *= (real32)(Input->dTime * Camera.LinearSpeed * SpeedMult);

	// Move the camera decimal point, add the floored integer part to the full Camera Position
	Camera.PositionDecimal += CameraMove;
	vec3i IntegerPos((int)floor(Camera.PositionDecimal.x), (int)floor(Camera.PositionDecimal.y), (int)floor(Camera.PositionDecimal.z));
	Camera.PositionDecimal -= IntegerPos;
	Camera.Position += IntegerPos;

	// UI for camera movement
	snprintf(State->Textline[1].String, UI_STRINGLEN, "Move : <%.4f, %.4f, %.4f>", CameraMove.x, CameraMove.y, CameraMove.z);

	//Camera.Position.y = Max(0.5f, Camera.Position.y); // Min at .5 meters

	if (MOUSE_HIT(Input->MouseRight))
	{
		Camera.FreeflyMode = true;
		State->DisableMouse.Switch();
		Camera.LastMousePos = MousePos;
	}
	if (MOUSE_RELEASED(Input->MouseRight))
	{
		Camera.FreeflyMode = false;
		State->DisableMouse.Switch();
	}

	if (Camera.FreeflyMode)
	{
		vec2i MouseOffset = MousePos - Camera.LastMousePos;
		Camera.LastMousePos = MousePos;

		if (MouseOffset.x != 0 || MouseOffset.y != 0)
		{
			Camera.Phi += real32(MouseOffset.x * Input->dTime * Camera.AngularSpeed);
			Camera.Theta += real32(MouseOffset.y * Input->dTime * Camera.AngularSpeed);

			if (Camera.Phi > M_TWO_PI) Camera.Phi -= M_TWO_PI;
			if (Camera.Phi < 0.0f) Camera.Phi += M_TWO_PI;

			Camera.Theta = std::max(1e-5f, std::min(real32(M_PI) - 1e-5f, Camera.Theta));
			Camera.Forward = SphericalToCartesian(Camera.Theta, Camera.Phi);

			Camera.Right = Normalize(Cross(Camera.Forward, vec3f(0, 1, 0)));
			Camera.Up = Normalize(Cross(Camera.Right, Camera.Forward));
		}
	}

	//Camera.Target = Camera.Position + Camera.Forward;
	vec3f CameraFullPosition = Camera.Position + Camera.PositionDecimal;
	Camera.ViewMatrix = mat4f::LookAtPrecise(CameraFullPosition, Camera.Forward, Camera.Right, Camera.Up);

	// Mouse cursor disabling when in Freefly
	if (State->DisableMouse.Switched())
	{
		if (State->DisableMouse)
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
	State->DayPhase = fmod(State->DayPhase + State->SunSpeed * M_PI * (real32)Input->dTime, 2.f * M_PI);


	real32 CosET = cosf(State->EarthTilt);

	vec3f SunPos(SunDistance * CosET * cosf(State->DayPhase) - EarthRadius * cosf(State->Latitude),
		SunDistance * sinf(State->EarthTilt) - EarthRadius * sinf(State->Latitude),
		SunDistance * CosET * sinf(State->DayPhase));
	mat4f Rot;
	Rot = Rot.RotateZ(M_PI_OVER_TWO - State->Latitude);
	SunPos = Rot * SunPos;

	//snprintf(Msg, CONSOLE_STRINGLEN, "%e %e %e", Rot[1][1], SunPos.y, SunPos.z); 
	//LogString(System->ConsoleLog, Msg);

	if (State->IsNight && SunPos.y > 0.f) {
		State->IsNight = false;
	}
	if (!State->IsNight && SunPos.y < 0.f) {
		State->IsNight = true;
	}

	State->SunDirection = Normalize(SunPos);
	//State->LightColor = vec4f(0.9f, 0.7, 0.8, 1.0f);
	State->LightColor = vec4f(2.7f, 2.1f, 2.4f, 1.0f);
}

void Update(state *State, rf::input *Input, rf::context *Context)
{
	State->Counter += Input->dTime;
	State->CounterTenth += Input->dTime;

	UpdateUI(State, Input, Context);

	MovePlayer(State, Input, Context);

	// Water state
#if 0
	if (KEY_DOWN(Input->Keys[KEY_KP_ADD]))
	{
		State->WaterStateInterp = State->WaterStateInterp + 0.01;

		if (State->WaterState < (water_system::BeaufortStateCount - 2))
		{
			if (State->WaterStateInterp >= 1.f)
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

	if (KEY_DOWN(Input->Keys[KEY_KP_SUBTRACT]))
	{
		State->WaterStateInterp = State->WaterStateInterp - 0.01;

		if (State->WaterState > 0)
		{
			if (State->WaterStateInterp < 0.f)
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
	if (Input->MouseDZ > 0)
	{
		// State->WaterDirection += Input->dTime * 0.05;
		State->SunSpeed += Input->MouseDZ * 0.0005f;
	}

	if (Input->MouseDZ < 0)
	{
		//State->WaterDirection -= Input->dTime * 0.05;
		State->SunSpeed += Input->MouseDZ * 0.0005f;
	}

	if (KEY_HIT(Input->Keys[KEY_P]))
		State->SunSpeed = 0.f;

	UpdateSky(State, Input);
}

void Destroy(state *State)
{
	(void)State;
}

}

#if 0

void FillAudioBuffer(tmp_sound_data *SoundData)
{
	uint32 ToneHz = 440;
	uint32 ALen = 40 * ToneHz;

	SoundData->SoundBufferSize = ALen;
	uint32 Size = SoundData->SoundBufferSize;

	for (uint32 i = 0; i < ALen; ++i)
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
	if (Input->KeyReleased)
	{
		tmp_sound_data *SoundData = System->SoundData;
		FillAudioBuffer(SoundData);
		SoundData->ReloadSoundBuffer = true;
	}
#endif


	UpdateSky(Local, State, System, Input);
}
#endif
