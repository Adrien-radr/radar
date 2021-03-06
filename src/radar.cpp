#include <cstdlib>
#include <string>
#include <algorithm>

#include "rf/utils.h"
#include "rf/context.h"
#include "rf/ui.h"

#include "definitions.h"

#include "Systems/water.h"
#include "Systems/atmosphere.h"
#include "Systems/planet.h"
#include "Game/sun.h"
#include "tests.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RF_WIN32
#include "radar_win32.cpp"
#elif RF_UNIX
#include "radar_unix.cpp"
#endif

#define DO_ATMOSPHERE 1
#define DO_WATER 0
#define DO_PLANET 0

memory *InitMemory()
{
    memory *Memory = (memory*)calloc(1, sizeof(memory));

	Memory->PermanentMemPool = rf::PoolCreate(32 * MB);
	Memory->SessionMemPool = rf::PoolCreate(512 * MB);
	Memory->ScratchMemPool = rf::PoolCreate(64 * MB);
	
    Memory->IsValid = Memory->PermanentMemPool && Memory->SessionMemPool && Memory->ScratchMemPool;

    return Memory;
}
void DestroyMemory(memory *Memory)
{
    if(Memory->IsValid)
    {
		rf::PoolFree(&Memory->PermanentMemPool);
		rf::PoolFree(&Memory->SessionMemPool);
		rf::PoolFree(&Memory->ScratchMemPool);
        Memory->IsValid = false;
    }
}

bool ParseConfig(config *ConfigOut, char const *Filename)
{
	path ExePath;
	rf::GetExecutablePath(ExePath);

	path ConfigPath;
	rf::ConcatStrings(ConfigPath, ExePath, Filename);

	void *Content = rf::ReadFileContentsNoContext(ConfigPath, 0);

	cJSON *root = nullptr;
	if (Content)
	{
		root = cJSON_Parse((char*)Content);
	}
	else
	{
		printf("Error parsing config file.\n");
	}

	ConfigOut->WindowX = rf::JSON_Get(root, "iWindowX", 200);
	ConfigOut->WindowY = rf::JSON_Get(root, "iWindowY", 200);
	ConfigOut->WindowWidth = rf::JSON_Get(root, "iWindowWidth", 960);
	ConfigOut->WindowHeight = rf::JSON_Get(root, "iWindowHeight", 540);
	ConfigOut->MSAA = rf::JSON_Get(root, "iMSAA", 0);
	ConfigOut->FullScreen = rf::JSON_Get(root, "bFullScreen", 0) != 0;
	ConfigOut->VSync = rf::JSON_Get(root, "bVSync", 0) != 0;
	ConfigOut->FOV = (real32)rf::JSON_Get(root, "fFOV", 75.0);
	ConfigOut->NearPlane = (real32)rf::JSON_Get(root, "fNearPlane", 0.1);
	ConfigOut->FarPlane = (real32)rf::JSON_Get(root, "fFarPlane", 10000.0);
	ConfigOut->AnisotropicFiltering = rf::JSON_Get(root, "iAnisotropicFiltering", 1);

	ConfigOut->CameraSpeedBase = (real32)rf::JSON_Get(root, "fCameraSpeedBase", 20.0);
	ConfigOut->CameraSpeedMult = rf::JSON_Get(root, "vCameraSpeedMult", vec4f(1));
	ConfigOut->CameraSpeedAngular = (real32)rf::JSON_Get(root, "fCameraSpeedAngular", 0.3);

	ConfigOut->CameraPosition = rf::JSON_Get(root, "vCameraPosition", vec3f(0, 6360001, 0));
	ConfigOut->CameraForward = rf::JSON_Get(root, "vCameraForward", vec3f(1, 0, 0));

	ConfigOut->TimeScale = (real32)rf::JSON_Get(root, "fTimescale", 30.0);

	if (Content) free(Content);

	return true;
}

void ReloadShaders(rf::context *Context)
{
    rf::ctx::RegisteredShaderClear(Context);
    path const &ExePath = rf::ctx::GetExePath(Context);

    path VSPath, FSPath;

    // TODO - TODO - PostProcessprogram shouldnt live in Context, this is application dependent
    rf::ConcatStrings(VSPath, ExePath, "data/shaders/screenquad_vert.glsl");
    rf::ConcatStrings(FSPath, ExePath, "data/shaders/hdr_frag.glsl");
    Context->ProgramPostProcess = BuildShader(Context, VSPath, FSPath);
    glUseProgram(Context->ProgramPostProcess);
    rf::SendInt(glGetUniformLocation(Context->ProgramPostProcess, "HDRFB"), 0);
    rf::CheckGLError("HDR Shader");

    Tests::ReloadShaders(Context);
    rf::ui::ReloadShaders(Context);

#if DO_WATER
    water::ReloadShaders(Context);
#endif
#if DO_ATMOSPHERE
    atmosphere::ReloadShaders(Context);
#endif
#if DO_PLANET
	planet::ReloadShaders(Context);
#endif
    rf::ctx::UpdateShaderProjection(Context);

    glUseProgram(0);
}

void MakeUI(memory *Memory, rf::context *Context, rf::input * /*Input*/)
{
    //game_system *System = (game_system*)Memory->PermanentMemPool;

#if 0
    console_log *Log = System->ConsoleLog;
    font *FontInfo = ui::GetFont(ui::FONT_DEFAULT);
    int32 LineGap = FontInfo->LineGap;
    int32 LogHeight = Context->WindowHeight/3;
    int32 LogCapacity = Log->StringCount - std::ceil((LogHeight) / (real32)LineGap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - Drop Down Console
    static bool   ConsoleShow = false;
    static uint32 ConsolePanel = 0;
    static vec3i  ConsolePanelPos(0,0,0);
    static vec2i  ConsolePanelSize(Context->WindowWidth, LogHeight);
    static real32 ConsoleSlider = 0.f;
    uint32 ConsoleDecorationFlags = ui::DECORATION_BORDER;

    if(KEY_HIT(Input->KeyTilde))
    {
        ConsoleShow = !ConsoleShow;
        ConsoleDecorationFlags |= ui::DECORATION_FOCUS; // force focus when opening the console
    }

    if(ConsoleShow)
    {
        ConsolePanelSize = vec2i(Context->WindowWidth, LogHeight);
        ui::BeginPanel(&ConsolePanel, "", &ConsolePanelPos, &ConsolePanelSize, ui::COLOR_CONSOLEBG, ConsoleDecorationFlags);
            ui::MakeSlider(&ConsoleSlider, 0.f, (real32)LogCapacity);
            for(uint32 i = 0; i < Log->StringCount; ++i)
            {
                uint32 RIdx = (Log->ReadIdx + i) % CONSOLE_CAPACITY;
                real32 YOffset = (real32)i + ConsoleSlider - (real32)Log->StringCount;
                real32 YPos = LogHeight + YOffset * LineGap;
                if(YPos < 0)
                    continue;
                if(YPos >= LogHeight)
                    break;
                vec2i Position(0, YPos);
                ui::MakeText((void*)Log->MsgStack[RIdx], Log->MsgStack[RIdx], ui::FONT_CONSOLE, Position,
                        ui::COLOR_CONSOLEFG, 1.f, Context->WindowWidth);
            }
        ui::EndPanel();
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - System Panel
    static bool SystemShow = false;

    vec2i UIStackPanelSize(360, 0);

    static uint32 SystemButtonPanel = 0;
    static char SystemButtonStr[16];
    static vec2i SystemButtonSize(25);
    vec3i SystemButtonPos(Context->WindowWidth-SystemButtonSize.x, 0, 0);
    static uint32 SystemButtonID = 0;
    snprintf(SystemButtonStr, 16, "%s", ICON_FA_COG);
    rf::ui::BeginPanel(&SystemButtonPanel, "", &SystemButtonPos, &SystemButtonSize, rf::ui::COLOR_PANELBG, rf::ui::DECORATION_INVISIBLE);
        if(rf::ui::MakeButton(&SystemButtonID, SystemButtonStr, rf::ui::FONT_AWESOME, vec2i(0), SystemButtonSize, 0.4f, rf::ui::DECORATION_BORDER))
        {
            SystemShow = !SystemShow;
        }
    rf::ui::EndPanel();

    static uint32 SystemPanelID = 0;
    static vec2i SystemPanelSize(210, 500);
    static vec3i SystemPanelPos(Context->WindowWidth - 50 - SystemPanelSize.x, 100, 0);

    if(SystemShow)
    {
        rf::ui::BeginPanel(&SystemPanelID, "System Info", &SystemPanelPos, &SystemPanelSize, rf::ui::COLOR_PANELBG);
            // Session Pool occupancy
            int CurrHeight = 0;
            real32 ToMiB = 1.f / (real32)MB;
            char OccupancyStr[64];

			static real32 PermanentOccupancy = 0.0f;
			PermanentOccupancy = rf::PoolOccupancy(Memory->PermanentMemPool);
			uint64 permanentUsedSpace = (uint64)(PermanentOccupancy * Memory->PermanentMemPool->Capacity);
            snprintf(OccupancyStr, 64, "permanent stack %.3f / %.2f MiB", permanentUsedSpace*ToMiB, Memory->PermanentMemPool->Capacity*ToMiB);
            rf::ui::MakeText(NULL, OccupancyStr, rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), rf::ui::COLOR_PANELFG);
            CurrHeight += 16;
            rf::ui::MakeProgressbar(&PermanentOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

			static real32 SessionOccupancy = 0.0f;
			SessionOccupancy = rf::PoolOccupancy(Memory->SessionMemPool);
			uint64 sessionUsedSpace = (uint64)(SessionOccupancy * Memory->SessionMemPool->Capacity);
            snprintf(OccupancyStr, 64, "session stack %.3f / %.2f MiB", sessionUsedSpace*ToMiB, Memory->SessionMemPool->Capacity*ToMiB);
            rf::ui::MakeText(NULL, OccupancyStr, rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), rf::ui::COLOR_PANELFG);
            CurrHeight += 16;
            rf::ui::MakeProgressbar(&SessionOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

			static real32 ScratchOccupancy = 0.0f;
			ScratchOccupancy = rf::PoolOccupancy(Memory->ScratchMemPool);
			uint64 scratchUsedSpace = (uint64)(ScratchOccupancy * Memory->ScratchMemPool->Capacity);
            snprintf(OccupancyStr, 64, "scratch stack %.3f / %.2f MiB", scratchUsedSpace*ToMiB, Memory->ScratchMemPool->Capacity*ToMiB);
            rf::ui::MakeText(NULL, OccupancyStr, rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), rf::ui::COLOR_PANELFG);
            CurrHeight += 16;
            rf::ui::MakeProgressbar(&ScratchOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

            static uint32 TmpBut = 0;
            rf::ui::MakeButton(&TmpBut, "a", rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), vec2i(20));
            CurrHeight += 26;

            static uint32 TmpBut2 = 0;
            rf::ui::MakeButton(&TmpBut2, "Hello", rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), vec2i(60,20));
            CurrHeight += 26;

            char Str[16];
            snprintf(Str, 16, "%s%s%s", ICON_FA_SEARCH, ICON_FA_GLASS, ICON_FA_SHARE);
            rf::ui::MakeText(NULL, Str, rf::ui::FONT_AWESOME, vec2i(0, 180), rf::ui::COLOR_BORDERBG);
        rf::ui::EndPanel();
    }
}

rf::context_descriptor MakeContextDescriptor(memory *Memory, config *Config, path const ExecutableName)
{
    rf::context_descriptor CtxDesc;
    CtxDesc.SessionPool = Memory->SessionMemPool;
    CtxDesc.ScratchPool = Memory->ScratchMemPool;
    // cpy WinX, WinY, WinW, WinH, VSync, FOV, NearPlane, FarPlane from Config to the Descriptor
    CtxDesc.WindowX = (real32)Config->WindowX;
    CtxDesc.WindowY = (real32)Config->WindowY;
    CtxDesc.WindowWidth = Config->WindowWidth;
    CtxDesc.WindowHeight = Config->WindowHeight;
    CtxDesc.VSync = Config->VSync;
    CtxDesc.FOV = Config->FOV;
    CtxDesc.NearPlane = Config->NearPlane;
    CtxDesc.FarPlane = Config->FarPlane;
    memcpy(CtxDesc.ExecutableName, ExecutableName, sizeof(path));

    return CtxDesc;
}

int RadarMain(int /*argc*/, char ** /*argv*/)
{
    // Init Engine memory
    memory *Memory = InitMemory();
    
    // Parse Engine config file (or get the default config if inexistent)
    config Config;
    if(!ParseConfig(&Config, "config.json"))
        return 1;

    // Create the RF context 
    path ExecutableName;
    snprintf(ExecutableName, MAX_PATH, "Radar Engine %d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);
    rf::context_descriptor CtxDesc = MakeContextDescriptor(Memory, &Config, ExecutableName);
    rf::context *Context = rf::ctx::Init(&CtxDesc);
    LogInfo("");
    LogInfo("%s", ExecutableName);

    if(!Context->IsValid || !Memory->IsValid)
        return 1;

    real64 CurrentTime, LastTime = glfwGetTime();
    real64 TimeCounter = 0.0;
    int const GameRefreshHz = 60;
    real64 const TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

    rf::BindTexture2D(0, 0);
    rf::CheckGLError("Engine Start");

    if(!Tests::Init(Context, &Config))
        return 1;

    int LastMouseX = 0, LastMouseY = 0;

    rf::mesh ScreenQuad = rf::Make2DQuad(Context, vec2i(-1,1), vec2i(1, -1));
    rf::frame_buffer FPBackbuffer = {};

    // TMP TextureViewer UI
    static uint32 TW_ID = 0;
    static real32 TW_ImgScale = 1.0f;
    static vec2f TW_ImgOffset(0.f, 0.f);
    static vec2i TW_Size(310, 330);
    static vec3i TW_Position(Context->WindowWidth - 10 - TW_Size.x, Context->WindowHeight - 10 - TW_Size.y, 0);

    // Game State initialization
    game::state *State = rf::PoolAlloc<game::state>(Memory->PermanentMemPool, 1);
    if(!game::Init(State, &Config))
    {
        LogError("Error initializing Game State.");
        return 1;
    }

    // Subsystems initialization
#if DO_ATMOSPHERE
    atmosphere::Init(State, Context);
#endif
#if DO_WATER
    water::Init(State, Context, State->WaterState);
#endif
#if DO_PLANET
	planet::Init(State, Context);
#endif

    // First time shader loading at the end of the initialization phase
    ReloadShaders(Context);

    while(Context->IsRunning)
    {
        rf::input Input = {};

        CurrentTime = glfwGetTime();
        Input.dTime = CurrentTime - LastTime;

        LastTime = CurrentTime;
        State->EngineTime += Input.dTime;
		State->dTime = Input.dTime;
        TimeCounter += Input.dTime;

        // NOTE - Each frame, clear the Scratch Arena Data
        // TODO - Is this too often ? Maybe let it stay several frames
        rf::PoolClear(Memory->ScratchMemPool);

        rf::ctx::GetFrameInput(Context, &Input);

        if(Context->HasResized)
        {
            // Resize FBO
            DestroyFramebuffer(&FPBackbuffer);
            FPBackbuffer = rf::MakeFramebuffer(1, vec2i(Context->WindowWidth, Context->WindowHeight));
            rf::FramebufferAttachBuffer(&FPBackbuffer, 0, 4, true, true, true); // RGBA16F attachment
            rf::CheckGLError("FramebufferAttach");
        }

        Input.MouseDX = Input.MousePosX - LastMouseX;
        Input.MouseDY = Input.MousePosY - LastMouseY;
        LastMouseX = Input.MousePosX;
        LastMouseY = Input.MousePosY;

        glClearColor(Context->ClearColor.x, Context->ClearColor.y, Context->ClearColor.z, Context->ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rf::ui::BeginFrame(&Input);

        game::Update(State, &Input, Context);

        // Local timed stuff
        if(TimeCounter > 0.1)
        {
            TimeCounter = 0.0;
        }

		// Additional termination test with escape key
		if (KEY_PRESSED(Input.Keys[KEY_ESCAPE]))
		{
			Context->IsRunning = false;
		}

        // Shader hot reload key (Shift-F11)
        if(KEY_PRESSED(Input.Keys[KEY_LEFT_SHIFT]) && KEY_RELEASED(Input.Keys[KEY_F11]))
        {
            ReloadShaders(Context);
        }

        // Wireframe toggle key (Shift-F10)
        if(KEY_PRESSED(Input.Keys[KEY_LEFT_SHIFT]) && KEY_RELEASED(Input.Keys[KEY_F10]))
        {
            rf::ctx::SetWireframeMode(Context);
        }


        glBindFramebuffer(GL_FRAMEBUFFER, FPBackbuffer.FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Tests::Render(State, &Input, Context);

#if DO_ATMOSPHERE
        atmosphere::Render(State, Context);
#endif
#if DO_WATER
        water::Update(State, &Input);
        water::Render(State, 0, 0);
#endif
#if DO_PLANET
		planet::Render(State, Context);
#endif

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Toggle wireframe off to draw the UI and the PostProcess pass
        GLenum CurrWireframemode = rf::ctx::SetWireframeMode(Context, GL_FILL);

        glUseProgram(Context->ProgramPostProcess);
        uint32 MipmapLogLoc = glGetUniformLocation(Context->ProgramPostProcess, "MipmapQueryLevel");
        uint32 ResolutionLoc = glGetUniformLocation(Context->ProgramPostProcess, "Resolution");
        rf::SendFloat(MipmapLogLoc, Context->WindowSizeLogLevel);
        rf::SendVec2(ResolutionLoc, vec2f((real32)Context->WindowWidth, (real32)Context->WindowHeight));
        rf::SendVec3(glGetUniformLocation(Context->ProgramPostProcess, "CameraPosition"), vec3f(State->Camera.Position + State->Camera.PositionDecimal));
        rf::BindTexture2D(FPBackbuffer.BufferIDs[0], 0);
        glGenerateMipmap(GL_TEXTURE_2D); // generate mipmap for the color buffer
        rf::CheckGLError("TexBind");

        glBindVertexArray(ScreenQuad.VAO);
        rf::RenderMesh(&ScreenQuad);

        MakeUI(Memory, Context, &Input);

#if 0
		rf::font *FontInfo = rf::ui::GetFont(rf::ui::FONT_AWESOME);
		rf::ui::BeginPanel(&TW_ID, "Texture Viewer", &TW_Position, &TW_Size, rf::ui::COLOR_PANELBG, rf::ui::DECORATION_TITLEBAR | rf::ui::DECORATION_BORDER);
		rf::ui::MakeImage(&TW_ImgScale, atmosphere::TransmittanceTexture/*FPBackbuffer.BufferIDs[0]*/, &TW_ImgOffset, vec2i(300, 300), false);
		rf::ui::EndPanel();
#endif

        rf::ui::Draw();

        rf::ctx::SetWireframeMode(Context, CurrWireframemode);

        glfwSwapBuffers(Context->Window);
    }

    rf::DestroyFramebuffer(&FPBackbuffer);
    Tests::Destroy();

    game::Destroy(State);
    rf::ctx::Destroy(Context);
    DestroyMemory(Memory);
    return 0;
}
