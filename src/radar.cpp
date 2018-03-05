#include <cstdlib>
#include <string>
#include <algorithm>

#include "rf/utils.h"
#include "rf/context.h"
#include "rf/ui.h"

#include "definitions.h"

#include "Systems/water.h"
#include "Systems/atmosphere.h"
#include "Game/sun.h"
#include "tests.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RF_WIN32
#include "radar_win32.cpp"
#elif RF_UNIX
#include "radar_unix.cpp"
#endif

memory *InitMemory()
{
    memory *Memory = (memory*)calloc(1, sizeof(memory));

    Memory->PermanentMemPoolSize = Megabytes(32);
    Memory->SessionMemPoolSize = Megabytes(512);
    Memory->ScratchMemPoolSize = Megabytes(64);

    Memory->PermanentMemPool = calloc(1, Memory->PermanentMemPoolSize);
    Memory->SessionMemPool = calloc(1, Memory->SessionMemPoolSize);
    Memory->ScratchMemPool = calloc(1, Memory->ScratchMemPoolSize);

    rf::InitArena(&Memory->PermanentArena, Memory->PermanentMemPoolSize, Memory->PermanentMemPool);
    rf::InitArena(&Memory->SessionArena, Memory->SessionMemPoolSize, Memory->SessionMemPool);
    rf::InitArena(&Memory->ScratchArena, Memory->ScratchMemPoolSize, Memory->ScratchMemPool);

    Memory->IsValid = Memory->PermanentMemPool && Memory->SessionMemPool && Memory->ScratchMemPool;

    return Memory;
}
void DestroyMemory(memory *Memory)
{
    if(Memory->IsValid)
    {
        free(Memory->PermanentMemPool);
        free(Memory->SessionMemPool);
        free(Memory->ScratchMemPool);
        Memory->PermanentMemPoolSize = 0;
        Memory->SessionMemPoolSize = 0;
        Memory->ScratchMemPoolSize = 0;
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
    if(Content)
    {
        cJSON *root = cJSON_Parse((char*)Content);
        if(root)
        {
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
            ConfigOut->CameraSpeedMult = (real32)rf::JSON_Get(root, "fCameraSpeedMult", 2.0);
            ConfigOut->CameraSpeedAngular = (real32)rf::JSON_Get(root, "fCameraSpeedAngular", 0.3);

            ConfigOut->CameraPosition = rf::JSON_Get(root, "vCameraPosition", vec3f(1,1,1));
            ConfigOut->CameraTarget = rf::JSON_Get(root, "vCameraTarget", vec3f(0,0,0));

            ConfigOut->TimeScale = rf::JSON_Get(root, "fTimescale", 30.0);
        }
        else
        {
            printf("Error parsing Config File as JSON.\n");
            return false;
        }
        free(Content);
    }
    else
    {
        printf("Generating default config...\n");
        path DefaultConfigPath;
        rf::ConcatStrings(DefaultConfigPath, ExePath, "default_config.json");

        // If the default config doesnt exist, just crash, someone has been stupid
        if(!rf::DiskFileExists(DefaultConfigPath))
        {
            printf("Fatal Error : Default Config file bin/default_config.json doesn't exist.\n");
            exit(1);
        }

        path PersonalConfigPath;
        rf::ConcatStrings(PersonalConfigPath, ExePath, "config.json");
        rf::DiskFileCopy(PersonalConfigPath, DefaultConfigPath);

        return ParseConfig(ConfigOut, DefaultConfigPath);
    }
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
    water::ReloadShaders(Context);
    atmosphere::ReloadShaders(Context);

    rf::ctx::UpdateShaderProjection(Context);

    glUseProgram(0);
}

void MakeUI(memory *Memory, rf::context *Context, rf::input *Input)
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
            real32 ToMiB = 1.f / (1024*1024);
            char OccupancyStr[64];

            static real32 PermanentOccupancy = Memory->PermanentArena.Size/(real64)Memory->PermanentArena.Capacity;
            snprintf(OccupancyStr, 64, "permanent stack %.2f / %.2f MiB", Memory->PermanentArena.Size*ToMiB, Memory->PermanentArena.Capacity*ToMiB);
            rf::ui::MakeText(NULL, OccupancyStr, rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), rf::ui::COLOR_PANELFG);
            CurrHeight += 16;
            rf::ui::MakeProgressbar(&PermanentOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

            static real32 SessionOccupancy = Memory->SessionArena.Size/(real64)Memory->SessionArena.Capacity;
            snprintf(OccupancyStr, 64, "session stack %.2f / %.2f MiB", Memory->SessionArena.Size*ToMiB, Memory->SessionArena.Capacity*ToMiB);
            rf::ui::MakeText(NULL, OccupancyStr, rf::ui::FONT_DEFAULT, vec2i(0, CurrHeight), rf::ui::COLOR_PANELFG);
            CurrHeight += 16;
            rf::ui::MakeProgressbar(&SessionOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

            static real32 ScratchOccupancy = Memory->ScratchArena.Size/(real64)Memory->ScratchArena.Capacity;
            snprintf(OccupancyStr, 64, "scratch stack %.2f / %.2f MiB", Memory->ScratchArena.Size*ToMiB, Memory->ScratchArena.Capacity*ToMiB);
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
    CtxDesc.SessionArena = &Memory->SessionArena;
    CtxDesc.ScratchArena = &Memory->ScratchArena;
    // cpy WinX, WinY, WinW, WinH, VSync, FOV, NearPlane, FarPlane from Config to the Descriptor
    CtxDesc.WindowX = Config->WindowX;
    CtxDesc.WindowY = Config->WindowY;
    CtxDesc.WindowWidth = Config->WindowWidth;
    CtxDesc.WindowHeight = Config->WindowHeight;
    CtxDesc.VSync = Config->VSync;
    CtxDesc.FOV = Config->FOV;
    CtxDesc.NearPlane = Config->NearPlane;
    CtxDesc.FarPlane = Config->FarPlane;
    memcpy(CtxDesc.ExecutableName, ExecutableName, sizeof(path));

    return CtxDesc;
}

int RadarMain(int argc, char **argv)
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
    game::state *State = PushArenaStruct(&Memory->PermanentArena, game::state);
    if(!game::Init(State, &Config))
    {
        LogError("Error initializing Game State.");
        return 1;
    }

    // Subsystems initialization
    atmosphere::Init(State, Context);
    water::Init(State, Context, State->WaterState);


    // First time shader loading at the end of the initialization phase
    ReloadShaders(Context);

    while(Context->IsRunning)
    {
        rf::input Input = {};

        CurrentTime = glfwGetTime();
        Input.dTime = CurrentTime - LastTime;

        LastTime = CurrentTime;
        State->EngineTime += Input.dTime;
        TimeCounter += Input.dTime;

        // NOTE - Each frame, clear the Scratch Arena Data
        // TODO - Is this too often ? Maybe let it stay several frames
        rf::ClearArena(&Memory->ScratchArena);

        rf::ctx::GetFrameInput(Context, &Input);

        if(rf::ctx::WindowResized(Context))
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

        // Shader hot reload key (Shift-F11)
        if(KEY_DOWN(Input.Keys[KEY_LEFT_SHIFT]) && KEY_UP(Input.Keys[KEY_F11]))
        {
            ReloadShaders(Context);
        }

        // Wireframe toggle key (Shift-F10)
        if(KEY_DOWN(Input.Keys[KEY_LEFT_SHIFT]) && KEY_UP(Input.Keys[KEY_F10]))
        {
            rf::ctx::SetWireframeMode(Context);
        }


        glBindFramebuffer(GL_FRAMEBUFFER, FPBackbuffer.FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Tests::Render(State, &Input, Context);

        atmosphere::Render(State, Context);
#if 1
        water::Update(State, &Input);
        water::Render(State, 0, 0);
#endif

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Toggle wireframe off to draw the UI and the PostProcess pass
        GLenum CurrWireframemode = rf::ctx::SetWireframeMode(Context, GL_FILL);

        glUseProgram(Context->ProgramPostProcess);
        uint32 MipmapLogLoc = glGetUniformLocation(Context->ProgramPostProcess, "MipmapQueryLevel");
        uint32 ResolutionLoc = glGetUniformLocation(Context->ProgramPostProcess, "Resolution");
        rf::SendFloat(MipmapLogLoc, Context->WindowSizeLogLevel);
        rf::SendVec2(ResolutionLoc, vec2f(Context->WindowWidth, Context->WindowHeight));
        rf::SendVec3(glGetUniformLocation(Context->ProgramPostProcess, "CameraPosition"), vec3f(State->Camera.Position.y));
        rf::BindTexture2D(FPBackbuffer.BufferIDs[0], 0);
        glGenerateMipmap(GL_TEXTURE_2D); // generate mipmap for the color buffer
        rf::CheckGLError("TexBind");

        glBindVertexArray(ScreenQuad.VAO);
        rf::RenderMesh(&ScreenQuad);

        MakeUI(Memory, Context, &Input);
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
#if 0
#if 1
            font *FontInfo = ui::GetFont(ui::FONT_AWESOME);
            ui::BeginPanel(&TW_ID, "Texture Viewer", &TW_Position, &TW_Size, ui::COLOR_PANELBG, ui::DECORATION_TITLEBAR | ui::DECORATION_BORDER);
            ui::MakeImage(&TW_ImgScale, Atmosphere::IrradianceTexture/*FPBackbuffer.BufferIDs[0]*/, &TW_ImgOffset, vec2i(300, 300), false);
            ui::EndPanel();
#endif
#endif
