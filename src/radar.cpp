#include <cstdlib>
#include <string>
#include <algorithm>

#include "utils.h"
#include "context.h"
#include "ui.h"
#include "water.h"
#include "atmosphere.h"
#include "sun.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RADAR_WIN32
#include "radar_win32.cpp"
#elif RADAR_UNIX
#include "radar_unix.cpp"
#endif

// Temporary rendering tests, to declutter this file
#include "tests.cpp"

game_memory *InitMemory()
{
    game_memory *Memory = (game_memory*)calloc(1, sizeof(game_memory));

    Memory->PermanentMemPoolSize = Megabytes(32);
    Memory->SessionMemPoolSize = Megabytes(512);
    Memory->ScratchMemPoolSize = Megabytes(64);

    Memory->PermanentMemPool = calloc(1, Memory->PermanentMemPoolSize);
    Memory->SessionMemPool = calloc(1, Memory->SessionMemPoolSize);
    Memory->ScratchMemPool = calloc(1, Memory->ScratchMemPoolSize);

    InitArena(&Memory->SessionArena, Memory->SessionMemPoolSize, Memory->SessionMemPool);
    InitArena(&Memory->ScratchArena, Memory->ScratchMemPoolSize, Memory->ScratchMemPool);

    Memory->ResourceHelper.Memory = Memory;
    GetExecutablePath(Memory->ResourceHelper.ExecutablePath);

    Memory->IsValid = Memory->PermanentMemPool && Memory->SessionMemPool && Memory->ScratchMemPool;
    Memory->IsInitialized = false;

    return Memory;
}

void DestroyMemory(game_memory *Memory)
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
        Memory->IsInitialized = false;
        Memory->IsGameInitialized = false;
    }
}

bool ParseConfig(game_memory *Memory, char const *Filename)
{
    game_config &Config = Memory->Config;

    path ConfigPath;
    MakeRelativePath(&Memory->ResourceHelper, ConfigPath, Filename);

    void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
    if(Content)
    {
        cJSON *root = cJSON_Parse((char*)Content);
        if(root)
        {
            Config.WindowX = JSON_Get(root, "iWindowX", 200);
            Config.WindowY = JSON_Get(root, "iWindowY", 200);
            Config.WindowWidth = JSON_Get(root, "iWindowWidth", 960);
            Config.WindowHeight = JSON_Get(root, "iWindowHeight", 540);
            Config.MSAA = JSON_Get(root, "iMSAA", 0);
            Config.FullScreen = JSON_Get(root, "bFullScreen", 0) != 0;
            Config.VSync = JSON_Get(root, "bVSync", 0) != 0;
            Config.FOV = (real32)JSON_Get(root, "fFOV", 75.0);
            Config.NearPlane = (real32)JSON_Get(root, "fNearPlane", 0.1);
            Config.FarPlane = (real32)JSON_Get(root, "fFarPlane", 10000.0);
            Config.AnisotropicFiltering = JSON_Get(root, "iAnisotropicFiltering", 1);

            Config.CameraSpeedBase = (real32)JSON_Get(root, "fCameraSpeedBase", 20.0);
            Config.CameraSpeedMult = (real32)JSON_Get(root, "fCameraSpeedMult", 2.0);
            Config.CameraSpeedAngular = (real32)JSON_Get(root, "fCameraSpeedAngular", 0.3);

            Config.CameraPosition = JSON_Get(root, "vCameraPosition", vec3f(1,1,1));
            Config.CameraTarget = JSON_Get(root, "vCameraTarget", vec3f(0,0,0));

            Config.TimeScale = JSON_Get(root, "fTimescale", 30.0);
        }
        else
        {
            printf("Error parsing Config File as JSON.\n");
            return false;
        }
    }
    else
    {
        printf("Generating default config...\n");
        path DefaultConfigPath;
        MakeRelativePath(&Memory->ResourceHelper, DefaultConfigPath, "default_config.json");

        // If the default config doesnt exist, just crash, someone has been stupid
        if(!DiskFileExists(DefaultConfigPath))
        {
            printf("Fatal Error : Default Config file bin/default_config.json doesn't exist.\n");
            exit(1);
        }

        path PersonalConfigPath;
        MakeRelativePath(&Memory->ResourceHelper, PersonalConfigPath, "config.json");
        DiskFileCopy(PersonalConfigPath, DefaultConfigPath);

        return ParseConfig(Memory, DefaultConfigPath);
    }
    return true;
}

void ReloadShaders(game_memory *Memory, game_context *Context)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;
    resource_helper *RH = &Memory->ResourceHelper;

    context::RegisteredShaderClear(Context);

    path VSPath, FSPath;

    MakeRelativePath(RH, VSPath, "data/shaders/screenquad_vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/hdr_frag.glsl");
    Context->ProgramPostProcess = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Context->ProgramPostProcess);
    SendInt(glGetUniformLocation(Context->ProgramPostProcess, "HDRFB"), 0);
    CheckGLError("HDR Shader");

    Tests::ReloadShaders(Memory, Context);
    ui::ReloadShaders(Memory, Context);
    Water::ReloadShaders(Memory, Context, System->WaterSystem);
    Atmosphere::ReloadShaders(Memory, Context);

    context::UpdateShaderProjection(Context);

    glUseProgram(0);
}

void InitializeFromGame(game_memory *Memory, game_context *Context)
{
    // Initialize Water from game Info
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    Water::Init(Memory, State, System, State->WaterState);
    Atmosphere::Init(Memory, Context, State, System);

    water_system *WaterSystem = System->WaterSystem;
    WaterSystem->VAO = MakeVertexArrayObject();
    WaterSystem->VBO[0] = AddIBO(GL_STATIC_DRAW, WaterSystem->IndexCount * sizeof(uint32), WaterSystem->IndexData);
    WaterSystem->VBO[1] = AddEmptyVBO(WaterSystem->VertexDataSize, GL_STATIC_DRAW);
    size_t VertSize = WaterSystem->VertexCount * sizeof(real32);
    FillVBO(0, 3, GL_FLOAT, 0, VertSize, WaterSystem->VertexData);
    FillVBO(1, 3, GL_FLOAT, VertSize, VertSize, WaterSystem->VertexData + WaterSystem->VertexCount);
    FillVBO(2, 3, GL_FLOAT, 2*VertSize, VertSize, WaterSystem->VertexData + 2 * WaterSystem->VertexCount);
    glBindVertexArray(0);

    Memory->IsGameInitialized = true;
    Memory->IsInitialized = true;
}

void MakeUI(game_memory *Memory, game_context *Context, game_input *Input)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - Frame Info Panel
    ui::frame_stack *UIStack = System->UIStack;
    int UIStackHeight = LineGap * UIStack->TextLineCount;
    static uint32 UIStackPanel = 0;
    static vec3i  UIStackPanelPos(0,0,0);
    static vec2i  UIStackPanelSize(360, UIStackHeight + 30);
    ui::BeginPanel(&UIStackPanel, "", &UIStackPanelPos, &UIStackPanelSize, ui::COLOR_PANELBG, ui::DECORATION_NONE);
        for(uint32 i = 0; i < UIStack->TextLineCount; ++i)
        {
            ui::text_line *Line = UIStack->TextLines[i];
            ui::MakeText((void*)Line, Line->String, ui::FONT_DEFAULT, Line->Position, Line->Color, 1.f, Context->WindowWidth);
        }
    ui::EndPanel();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - System Panel
    static bool SystemShow = false;

    static uint32 SystemButtonPanel = 0;
    static char SystemButtonStr[16];
    static vec3i SystemButtonPos(UIStackPanelSize.x, 0, 0);
    static vec2i SystemButtonSize(25);
    static uint32 SystemButtonID = 0;
    snprintf(SystemButtonStr, 16, "%s", ICON_FA_COG);
    ui::BeginPanel(&SystemButtonPanel, "", &SystemButtonPos, &SystemButtonSize, ui::COLOR_PANELBG, ui::DECORATION_INVISIBLE);
        if(ui::MakeButton(&SystemButtonID, SystemButtonStr, ui::FONT_AWESOME, vec2i(0), SystemButtonSize, 0.4f, ui::DECORATION_BORDER))
        {
            SystemShow = !SystemShow;
        }
    ui::EndPanel();

    static uint32 SystemPanelID = 0;
    static vec2i SystemPanelSize(210, 500);
    static vec3i SystemPanelPos(Context->WindowWidth - 50 - SystemPanelSize.x, 100, 0);

    if(SystemShow)
    {
        ui::BeginPanel(&SystemPanelID, "System Info", &SystemPanelPos, &SystemPanelSize, ui::COLOR_PANELBG);
            // Session Pool occupancy
            int CurrHeight = 0;
            real32 ToMiB = 1.f / (1024*1024);
            char OccupancyStr[32];
            static real32 SessionOccupancy = Memory->SessionArena.Size/(real64)Memory->SessionArena.Capacity;
            snprintf(OccupancyStr, 32, "session stack %.1f / %.1f MiB", Memory->SessionArena.Size*ToMiB, Memory->SessionArena.Capacity*ToMiB);
            ui::MakeText(NULL, OccupancyStr, ui::FONT_DEFAULT, vec2i(0, CurrHeight), ui::COLOR_PANELFG);
            CurrHeight += 16;
            ui::MakeProgressbar(&SessionOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

            static real32 ScratchOccupancy = Memory->ScratchArena.Size/(real64)Memory->ScratchArena.Capacity;
            snprintf(OccupancyStr, 32, "scratch stack %.1f / %.1f MiB", Memory->ScratchArena.Size*ToMiB, Memory->ScratchArena.Capacity*ToMiB);
            ui::MakeText(NULL, OccupancyStr, ui::FONT_DEFAULT, vec2i(0, CurrHeight), ui::COLOR_PANELFG);
            CurrHeight += 16;
            ui::MakeProgressbar(&ScratchOccupancy, 1.f, vec2i(0, CurrHeight), vec2i(300, 10));
            CurrHeight += 16;

            static uint32 TmpBut = 0;
            ui::MakeButton(&TmpBut, "a", ui::FONT_DEFAULT, vec2i(0, CurrHeight), vec2i(20));
            CurrHeight += 26;

            static uint32 TmpBut2 = 0;
            ui::MakeButton(&TmpBut2, "Hello", ui::FONT_DEFAULT, vec2i(0, CurrHeight), vec2i(60,20));
            CurrHeight += 26;

            char Str[16];
            snprintf(Str, 16, "%s%s%s", ICON_FA_SEARCH, ICON_FA_GLASS, ICON_FA_SHARE);
            ui::MakeText(NULL, Str, ui::FONT_AWESOME, vec2i(0, 180), ui::COLOR_BORDERBG);
        ui::EndPanel();
    }

}

int RadarMain(int argc, char **argv)
{
    path DllSrcPath;
    path DllDstPath;

    game_memory *Memory = InitMemory();
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    rlog::Init(Memory);

    if(!ParseConfig(Memory, "config.json"))
        return 1;

    System->SoundData = (tmp_sound_data*)PushArenaStruct(&Memory->SessionArena, tmp_sound_data);

    MakeRelativePath(&Memory->ResourceHelper, DllSrcPath, DllName);
    MakeRelativePath(&Memory->ResourceHelper, DllDstPath, DllDynamicCopyName);

    game_context *Context = context::Init(Memory);
    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_config const &Config = Memory->Config;

    if(Context->IsValid && Memory->IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        real64 TimeCounter = 0.0;
        int const GameRefreshHz = 60;
        real64 const TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        ui::Init(Memory, Context);

        BindTexture2D(0, 0);
        CheckGLError("Start");

        if(!Tests::Init(Context, Config))
            return 1;

        bool LastDisableMouse = false;
        int LastMouseX = 0, LastMouseY = 0;

        mesh ScreenQuad = Make2DQuad(vec2i(-1,1), vec2i(1, -1));
        frame_buffer FPBackbuffer = {};

        // TMP TextureViewer UI
        static uint32 TW_ID = 0;
        static real32 TW_ImgScale = 1.0f;
        static vec2f TW_ImgOffset(0.f, 0.f);
        static vec2i TW_Size(310, 330);
        static vec3i TW_Position(Context->WindowWidth - 10 - TW_Size.x, Context->WindowHeight - 10 - TW_Size.y, 0);

        while(Context->IsRunning)
        {
            game_input Input = {};

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            LastTime = CurrentTime;
            State->EngineTime += Input.dTime;
            TimeCounter += Input.dTime;

            // NOTE - Each frame, clear the Scratch Arena Data
            // TODO - Is this too often ? Maybe let it stay several frames
            ClearArena(&Memory->ScratchArena);

            context::GetFrameInput(Context, &Input);

            if(context::WindowResized(Context))
            {
                // Resize FBO
                DestroyFramebuffer(&FPBackbuffer);
                FPBackbuffer = MakeFramebuffer(1, vec2i(Context->WindowWidth, Context->WindowHeight));
                FramebufferAttachBuffer(&FPBackbuffer, 0, 4, true, true, true); // RGBA16F attachment
                CheckGLError("FramebufferAttach");
            }

            Input.MouseDX = Input.MousePosX - LastMouseX;
            Input.MouseDY = Input.MousePosY - LastMouseY;
            LastMouseX = Input.MousePosX;
            LastMouseY = Input.MousePosY;

            if(CheckNewDllVersion(&Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            glClearColor(Context->ClearColor.x, Context->ClearColor.y, Context->ClearColor.z, Context->ClearColor.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ui::BeginFrame(Memory, &Input);

            Game.GameUpdate(Memory, &Input);

            if(!Memory->IsInitialized)
            {
                InitializeFromGame(Memory, Context);
                ReloadShaders(Memory, Context);			// First Shader loading after the game is initialize
            }

            // Local timed stuff
            if(TimeCounter > 0.1)
            {
                TimeCounter = 0.0;
            }

            if(LastDisableMouse != State->DisableMouse)
            {
                if(State->DisableMouse)
                {
                    glfwSetInputMode(Context->Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                else
                {
                    glfwSetInputMode(Context->Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }

                LastDisableMouse = State->DisableMouse;
            }

            if(KEY_DOWN(Input.KeyLShift) && KEY_UP(Input.KeyF11))
            {
                ReloadShaders(Memory, Context);
            }

            if(KEY_UP(Input.KeyF1))
            {
                context::SetWireframeMode(Context); // toggle
            }

            glBindFramebuffer(GL_FRAMEBUFFER, FPBackbuffer.FBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Tests::Render(Context, State, &Input);

#if 0
            //Water::Update(State, System->WaterSystem, &Input);
            Water::Render(State, System->WaterSystem, HDRGlossyEnvmap, GGXLUT);
#endif

            Atmosphere::Render(State, Context);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            GLenum CurrWireframemode = context::SetWireframeMode(Context, GL_FILL);

            glUseProgram(Context->ProgramPostProcess);
            uint32 MipmapLogLoc = glGetUniformLocation(Context->ProgramPostProcess, "MipmapQueryLevel");
            uint32 ResolutionLoc = glGetUniformLocation(Context->ProgramPostProcess, "Resolution");
            SendFloat(MipmapLogLoc, Context->WindowSizeLogLevel);
            SendVec2(ResolutionLoc, vec2f(Context->WindowWidth, Context->WindowHeight));
            SendVec3(glGetUniformLocation(Context->ProgramPostProcess, "CameraPosition"), vec3f(State->Camera.Position.y));
            BindTexture2D(FPBackbuffer.BufferIDs[0], 0);
            glGenerateMipmap(GL_TEXTURE_2D); // generate mipmap for the color buffer
            CheckGLError("TexBind");

            glBindVertexArray(ScreenQuad.VAO);
            RenderMesh(&ScreenQuad);

#if 1
            font *FontInfo = ui::GetFont(ui::FONT_AWESOME);
            ui::BeginPanel(&TW_ID, "Texture Viewer", &TW_Position, &TW_Size, ui::COLOR_PANELBG, ui::DECORATION_TITLEBAR | ui::DECORATION_BORDER);
            ui::MakeImage(&TW_ImgScale, Atmosphere::IrradianceTexture/*FPBackbuffer.BufferIDs[0]*/, &TW_ImgOffset, vec2i(300, 300), false);
            ui::EndPanel();
#endif

            MakeUI(Memory, Context, &Input);
            ui::Draw();

            context::SetWireframeMode(Context, CurrWireframemode);

            glfwSwapBuffers(Context->Window);
        }

        DestroyFramebuffer(&FPBackbuffer);
        Tests::Destroy();
    }

    context::Destroy(Context);
    UnloadGameCode(&Game, DllDstPath);
    rlog::Destroy();
    DestroyMemory(Memory);

    return 0;
}