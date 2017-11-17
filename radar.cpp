#include <cstdlib>
#include <string>
#include <algorithm>

#include "utils.h"
#include "context.h"
#include "ui.h"
#include "water.h"
#include "sun.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RADAR_WIN32
#include "radar_win32.cpp"
#elif RADAR_UNIX
#include "radar_unix.cpp"
#endif

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

bool ParseConfig(game_memory *Memory, char *ConfigPath)
{
    game_config &Config = Memory->Config;

    void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
    if(Content)
    {
        cJSON *root = cJSON_Parse((char*)Content);
        if(root)
        {
            // TODO - Holy shit this is ugly. maybe do it C style instead of dirty templates
            std::string tmp = JSON_Get(root, "sLogFile", std::string("radar.log"));
            strncpy(Config.LogFilename, tmp.c_str(), MAX_PATH);

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

uint32 Program1, Program3D, ProgramSkybox;
uint32 ProgramHDR;

void ReloadShaders(game_memory *Memory, game_context *Context)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;
    resource_helper *RH = &Memory->ResourceHelper;

    context::RegisteredShaderClear(Context);

    path VSPath;
    path FSPath;
    MakeRelativePath(RH, VSPath, "data/shaders/text_vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/text_frag.glsl");
    Program1 = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program1);
    SendInt(glGetUniformLocation(Program1, "DiffuseTexture"), 0);
    CheckGLError("Text Shader");

    context::RegisterShader2D(Context, Program1);

    MakeRelativePath(RH, VSPath, "data/shaders/vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/frag.glsl");
    Program3D = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program3D);
    SendInt(glGetUniformLocation(Program3D, "Albedo"), 0);
    SendInt(glGetUniformLocation(Program3D, "MetallicRoughness"), 1);
    SendInt(glGetUniformLocation(Program3D, "GGXLUT"), 2);
    SendInt(glGetUniformLocation(Program3D, "Skybox"), 3);
    SendInt(glGetUniformLocation(Program3D, "IrradianceCubemap"), 4);
    CheckGLError("Mesh Shader");

    context::RegisterShader3D(Context, Program3D);

    MakeRelativePath(RH, VSPath, "data/shaders/skybox_vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/skybox_frag.glsl");
    ProgramSkybox = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramSkybox);
    SendInt(glGetUniformLocation(ProgramSkybox, "Skybox"), 0);
    CheckGLError("Skybox Shader");

    context::RegisterShader3D(Context, ProgramSkybox);

    MakeRelativePath(RH, VSPath, "data/shaders/water_vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/water_frag.glsl");
    System->WaterSystem->ProgramWater = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(System->WaterSystem->ProgramWater);
    SendInt(glGetUniformLocation(System->WaterSystem->ProgramWater, "Skybox"), 0);
    SendInt(glGetUniformLocation(System->WaterSystem->ProgramWater, "IrradianceCubemap"), 1);
    CheckGLError("Water Shader");

    context::RegisterShader3D(Context, System->WaterSystem->ProgramWater);

    MakeRelativePath(RH, VSPath, "data/shaders/screenquad_vert.glsl");
    MakeRelativePath(RH, FSPath, "data/shaders/hdr_frag.glsl");
    ProgramHDR = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramHDR);
    SendInt(glGetUniformLocation(ProgramHDR, "HDRFB"), 0);
    CheckGLError("HDR Shader");

    ui::ReloadShaders(Memory, Context);
    CheckGLError("UI Shader");

    context::UpdateShaderProjection(Context);

    glUseProgram(0);
}

void InitializeFromGame(game_memory *Memory)
{
    // Initialize Water from game Info
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    Water::Initialization(Memory, State, System, State->WaterState);

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
    static vec2i  UIStackPanelSize(350, UIStackHeight + 30);
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
    static vec3i SystemButtonPos(350, 0, 0);
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

    path ConfigPath;
    MakeRelativePath(&Memory->ResourceHelper, ConfigPath, "config.json");

    if(!ParseConfig(Memory, ConfigPath))
    {
        return 1;
    }

    rlog::Init(Memory);
    System->SoundData = (tmp_sound_data*)PushArenaStruct(&Memory->SessionArena, tmp_sound_data);

    MakeRelativePath(&Memory->ResourceHelper, DllSrcPath, DllName);
    MakeRelativePath(&Memory->ResourceHelper, DllDstPath, DllDynamicCopyName);

    game_context *Context = context::Init(Memory);
    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_config const &Config = Memory->Config;

    if(Context->IsValid && Memory->IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        ui::Init(Memory, Context);

        glActiveTexture(GL_TEXTURE0);
        CheckGLError("Start");
/////////////////////////
    // TEMP TESTS
#if 0
        ALuint AudioBuffer;
        ALuint AudioSource;
        if(TempPrepareSound(&AudioBuffer, &AudioSource))
        {
            alSourcePlay(AudioSource);
        }
#endif

        // Texture Test
        //image *Image = ResourceLoadImage(&Context.RenderResources, "data/crate1_diffuse.png", false);
        uint32 *Texture1 = ResourceLoad2DTexture(&Context->RenderResources, "data/crate1_diffuse.png",
                false, false, Config.AnisotropicFiltering);

        uint32 *PBRTex0_Albedo = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/StreakedMetal/albedo.png",
                false, false, Config.AnisotropicFiltering);
        uint32 *PBRTex0_MetalRoughness = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/StreakedMetal/metalroughness.png",
                false, false, Config.AnisotropicFiltering);
#if 0
        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/albedo.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalAlbedo = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/metallic.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalMetallic = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/roughness.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalRoughness = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/normal.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalNormal = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);
#endif

        // Cube Meshes Test
        mesh Cube = MakeUnitCube();
        mesh Sphere = MakeUnitSphere(true, 2);
        mesh SkyboxCube = MakeUnitCube(false);

        real32 Dim = 20.0f;
        vec3f LowDim = -Dim/2;
        vec3f CubePos[] = {
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX)
        };
        vec3f CubeRot[] = {
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX)
        };

        uint32 HDRCubemapEnvmap, HDRIrradianceEnvmap, HDRGlossyEnvmap;
        ComputeIrradianceCubemap(&Context->RenderResources, "data/envmap_monument.hdr", &HDRCubemapEnvmap, &HDRGlossyEnvmap, &HDRIrradianceEnvmap);
        uint32 EnvmapToUse = HDRCubemapEnvmap;

        uint32 GGXLUT = PrecomputeGGXLUT(&Context->RenderResources, 512);

        model gltfCube = {};
        mesh defCube = MakeUnitCube();
        //if(!ResourceLoadGLTFModel(&gltfCube, "data/gltftest/PBRSpheres/MetalRoughSpheres.gltf", &Context))
        if(!ResourceLoadGLTFModel(&Context->RenderResources, &gltfCube, "data/gltftest/suzanne/Suzanne.gltf", Context))
            return 1;

        bool LastDisableMouse = false;
        int LastMouseX = 0, LastMouseY = 0;

        mesh ScreenQuad = Make2DQuad(vec2i(-1,1), vec2i(1, -1));
        frame_buffer FPBackbuffer;

        real64 TimeCounter = 0.0;

        // TMP
        static uint32 id2 = 0;
        static real32 imgscale = 1.0f;
        static vec2f img_texoffset(0.f, 0.f);
        static vec2i p2size(310, 330);
        static vec3i p2(Context->WindowWidth - 10 - p2size.x, Context->WindowHeight - 10 - p2size.y, 0);

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
                InitializeFromGame(Memory);
                ReloadShaders(Memory, Context);			// First Shader loading after the game is initialize
            }

            // Local timed stuff
            if(TimeCounter > 0.1)
            {
                TimeCounter = 0.0;
            }

#if 0 // NOTE - Sound play
                tmp_sound_data *SoundData = System->SoundData;
                if(SoundData->ReloadSoundBuffer)
                {
                    SoundData->ReloadSoundBuffer = false;
                    alSourceStop(AudioSource);
                    alSourcei(AudioSource, AL_BUFFER, 0);
                    alBufferData(AudioBuffer, AL_FORMAT_MONO16, SoundData->SoundBuffer, SoundData->SoundBufferSize, 48000);
                    alSourcei(AudioSource, AL_BUFFER, AudioBuffer);
                    alSourcePlay(AudioSource);
                    CheckALError();
                }
#endif

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

            if(KEY_UP(Input.KeyF2))
            {
                if(EnvmapToUse == HDRCubemapEnvmap)
                    EnvmapToUse = HDRIrradianceEnvmap;
                else
                    EnvmapToUse = HDRCubemapEnvmap;
            }


            // Recompute Camera View matrix if needed
            game_camera &Camera = State->Camera;
            mat4f &ViewMatrix = Camera.ViewMatrix;
            ViewMatrix = mat4f::LookAt(Camera.Position, Camera.Target, Camera.Up);

            // 1 . Render the scene on the HDR Floating point FBO
            glBindFramebuffer(GL_FRAMEBUFFER, FPBackbuffer.FBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0 // NOTE - Model rendering test
            {
                glCullFace(GL_BACK);
                glUseProgram(Program3D);
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }

                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);
                uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
                uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
                uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
                for(uint32 i = 0; i < gltfCube.Mesh.size(); ++i)
                {
                    material const &Mat = gltfCube.Material[gltfCube.MaterialIdx[i]];

                    SendVec3(AlbedoLoc, Mat.AlbedoMult);
                    SendFloat(MetallicLoc, Mat.MetallicMult);
                    SendFloat(RoughnessLoc, Mat.RoughnessMult);
                    glBindVertexArray(gltfCube.Mesh[i].VAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Mat.AlbedoTexture);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Mat.RoughnessMetallicTexture);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, EnvmapToUse);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, HDRIrradianceEnvmap);
                    mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
                    Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                    ModelMatrix.FromTRS(vec3f(0), vec3f(0), vec3f(2));
                    SendMat4(Loc, ModelMatrix);
                    glDrawElements(GL_TRIANGLES, gltfCube.Mesh[i].IndexCount, gltfCube.Mesh[i].IndexType, 0);
                }
            }
#endif

#if 0 // NOTE - CUBE DRAWING Test Put somewhere else
            {
                glUseProgram(Program3D);
                {

                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }

                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);
                glBindVertexArray(Cube.VAO);
                glBindTexture(GL_TEXTURE_2D, Texture1);
                mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                for(uint32 i = 0; i < 5; ++i)
                {
                    ModelMatrix.FromTRS(CubePos[i], CubeRot[i], vec3f(1.f));
                    SendMat4(Loc, ModelMatrix);
                    CheckGLError("ModelMatrix");
                    glDrawElements(GL_TRIANGLES, Cube.IndexCount, Cube.IndexType, 0);
                }

                real32 hW = PlaneWidth/2.f;
                ModelMatrix.FromTRS(vec3f(-hW, -7.f, -hW), vec3f(0), vec3f(1));
                SendMat4(Loc, ModelMatrix);
                glBindVertexArray(UnderPlane.VAO);
                glDrawElements(GL_TRIANGLES, UnderPlane.IndexCount, UnderPlane.IndexType, 0);
            }
#endif

#if 1
            { // NOTE - Sphere Array Test for PBR
                glUseProgram(Program3D);
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }
                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);

                glBindVertexArray(Sphere.VAO);
                mat4f ModelMatrix;
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, GGXLUT);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRIrradianceEnvmap);

                int Count = 5;
                uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
                uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
                uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
                SendVec3(AlbedoLoc, vec3f(1, 0.3, 0.7));
                for(int j = 0; j < Count; ++j)
                {
                    SendFloat(MetallicLoc, (j)/(real32)Count);
                    for(int i = 0; i < Count; ++i)
                    {
                        SendFloat(RoughnessLoc, Clamp((i)/(real32)Count, 0.05f, 1.f));
                        ModelMatrix.FromTRS(vec3f(0, 3*(j+1), 3*(i+1)), vec3f(0.f), vec3f(1.f));
                        SendMat4(Loc, ModelMatrix);
                        glDrawElements(GL_TRIANGLES, Sphere.IndexCount, Sphere.IndexType, 0);
                    }
                }
                glActiveTexture(GL_TEXTURE0);
            }
#endif

#if 1
            { // NOTE - PBR materials rendering tests
                glUseProgram(Program3D);
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                }
                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);

                glBindVertexArray(Sphere.VAO);
                mat4f ModelMatrix;
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, *PBRTex0_Albedo);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, *PBRTex0_MetalRoughness);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, GGXLUT);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRIrradianceEnvmap);
                uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
                uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
                uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
                SendVec3(AlbedoLoc, vec3f(1));
                SendFloat(MetallicLoc, 1.0);
                SendFloat(RoughnessLoc, 1.0);
                ModelMatrix.FromTRS(vec3f(-3.0, 3.0, 3.0), vec3f(0.f), vec3f(1.f));
                SendMat4(Loc, ModelMatrix);
                glDrawElements(GL_TRIANGLES, Sphere.IndexCount, Sphere.IndexType, 0);
                CheckGLError("PBR draw");

                glActiveTexture(GL_TEXTURE0);
            }
#endif

#if 1
            //Water::Update(State, System->WaterSystem, &Input);
            Water::Render(State, System->WaterSystem, EnvmapToUse, HDRIrradianceEnvmap);
#endif

            { // NOTE - Skybox Rendering Test, put somewhere else
                glDisable(GL_CULL_FACE);
                glDepthFunc(GL_LEQUAL);
                CheckGLError("Skybox");

                glUseProgram(ProgramSkybox);
                {
                    // NOTE - remove translation component from the ViewMatrix for the skybox
                    mat4f SkyViewMatrix = ViewMatrix;
                    SkyViewMatrix.SetTranslation(vec3f(0.f));
                    uint32 Loc = glGetUniformLocation(ProgramSkybox, "ViewMatrix");
                    SendMat4(Loc, SkyViewMatrix);
                    CheckGLError("ViewMatrix Skybox");
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, EnvmapToUse);
                glBindVertexArray(SkyboxCube.VAO);
                glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, SkyboxCube.IndexType, 0);

                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            CheckGLError("FBO Bind");

            //GLenum CurrWireframemode = context::SetWireframeMode(Context, GL_FILL);

            // Draw the floatingpoint FBO to a screen quad
            glUseProgram(ProgramHDR);
            uint32 MipmapLogLoc = glGetUniformLocation(ProgramHDR, "MipmapQueryLevel");
            uint32 ResolutionLoc = glGetUniformLocation(ProgramHDR, "Resolution");
            SendFloat(MipmapLogLoc, Context->WindowSizeLogLevel);
            SendVec2(ResolutionLoc, vec2f(Context->WindowWidth, Context->WindowHeight));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, FPBackbuffer.BufferIDs[0]);
            glGenerateMipmap(GL_TEXTURE_2D); // generate mipmap for the color buffer
            CheckGLError("TexBind");

            glBindVertexArray(ScreenQuad.VAO);
            glDrawElements(GL_TRIANGLES, ScreenQuad.IndexCount, ScreenQuad.IndexType, 0);


#if 1
            font *FontInfo = ui::GetFont(ui::FONT_AWESOME);
            ui::BeginPanel(&id2, "Panel 2", &p2, &p2size, ui::COLOR_PANELBG, ui::DECORATION_TITLEBAR | ui::DECORATION_BORDER);
            ui::MakeImage(&imgscale, GGXLUT, &img_texoffset, vec2i(300, 300), false);
            ui::EndPanel();
#endif

            MakeUI(Memory, Context, &Input);
            ui::Draw();

            //context::SetWireframeMode(Context, CurrWireframemode);

            glfwSwapBuffers(Context->Window);
        }

        // TODO - Destroy Console meshes
        DestroyMesh(&Cube);
        DestroyMesh(&SkyboxCube);
        DestroyFramebuffer(&FPBackbuffer);
        glDeleteProgram(Program1);
        glDeleteProgram(Program3D);
        glDeleteProgram(ProgramSkybox);
    }

    context::Destroy(Context);
    UnloadGameCode(&Game, DllDstPath);
    rlog::Destroy();
    DestroyMemory(Memory);

    return 0;
}
