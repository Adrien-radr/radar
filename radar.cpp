#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sun.h"
#include "radar.h"

int RadarMain(int argc, char **argv);
void *ReadFileContents(memory_arena *Arena, char *Filename, int *FileSize);
void MakeRelativePath(char *Dst, char *Path, char const *Filename);

// PLATFORM
#if RADAR_WIN32
#include "radar_win32.cpp"
#elif RADAR_UNIX
#include "radar_unix.cpp"
#endif

// EXTERNAL
#include "cJSON.h"

// IMPLEMENTATION
#include "render.cpp"
#include "sound.cpp"
#include "water.cpp"

path ExecFullPath;

bool FramePressedKeys[350] = {};
bool FrameReleasedKeys[350] = {};
bool FrameDownKeys[350] = {};

int  FrameModKeys = 0;

bool FramePressedMouseButton[8] = {};
bool FrameDownMouseButton[8] = {};
bool FrameReleasedMouseButton[8] = {};

int  FrameMouseWheel = 0;

// TODO - For resizing from GLFW callbacks. Is there a better way to do this ?
int ResizeWidth;
int ResizeHeight;
bool Resized = false;

struct game_context
{
    GLFWwindow *Window;
    real64 EngineTime;

    mat4f ProjectionMatrix3D;
    mat4f ProjectionMatrix2D;

    bool WireframeMode;
    vec4f ClearColor;

    uint32 DefaultDiffuseTexture;
    uint32 DefaultNormalTexture;

    font   DefaultFont;
    uint32 DefaultFontTexture;

    real32 FOV;
    int WindowWidth;
    int WindowHeight;

    bool IsRunning;
    bool IsValid;
};

#include "ui.cpp"

game_memory InitMemory()
{
    game_memory Memory = {};

    Memory.PermanentMemPoolSize = Megabytes(32);
    Memory.SessionMemPoolSize = Megabytes(512);
    Memory.ScratchMemPoolSize = Megabytes(64);

    Memory.PermanentMemPool = calloc(1, Memory.PermanentMemPoolSize);
    Memory.SessionMemPool = calloc(1, Memory.SessionMemPoolSize);
    Memory.ScratchMemPool = calloc(1, Memory.ScratchMemPoolSize);

    InitArena(&Memory.SessionArena, Memory.SessionMemPoolSize, Memory.SessionMemPool);
    InitArena(&Memory.ScratchArena, Memory.ScratchMemPoolSize, Memory.ScratchMemPool);

    Memory.IsValid = Memory.PermanentMemPool && Memory.SessionMemPool && Memory.ScratchMemPool;
    Memory.IsInitialized = false;

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

void MakeRelativePath(char *Dst, char *Path, char const *Filename)
{
    strncpy(Dst, Path, MAX_PATH);
    strncat(Dst, Filename, MAX_PATH);
}

void *ReadFileContents(memory_arena *Arena, char *Filename, int32 *FileSize)
{
    char *Contents = NULL;
    FILE *fp = fopen(Filename, "rb");

    if(fp)
    {
        if(0 == fseek(fp, 0, SEEK_END))
        {
            int32 Size = ftell(fp);
            rewind(fp);
            Contents = (char*)PushArenaData(Arena, Size+1);
            fread(Contents, Size, 1, fp);
            Contents[Size] = 0;
            if(FileSize)
            {
                *FileSize = Size+1;
            }
        }
        else
        {
            printf("File Open Error : fseek not 0.\n");
        }
        fclose(fp);
    }
    else
    {
        printf("Coudln't open file %s.\n", Filename);
    }

    return (void*)Contents;
}

void ParseConfig(game_memory *Memory, char *ConfigPath)
{
    game_config &Config = Memory->Config;

    void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
    if(Content)
    {
        cJSON *root = cJSON_Parse((char*)Content);
        if(root)
        {
            Config.WindowWidth = cJSON_GetObjectItem(root, "iWindowWidth")->valueint;
            Config.WindowHeight = cJSON_GetObjectItem(root, "iWindowHeight")->valueint;
            Config.MSAA = cJSON_GetObjectItem(root, "iMSAA")->valueint;
            Config.FullScreen = cJSON_GetObjectItem(root, "bFullScreen")->valueint != 0;
            Config.VSync = cJSON_GetObjectItem(root, "bVSync")->valueint != 0;
            Config.FOV = (real32)cJSON_GetObjectItem(root, "fFOV")->valuedouble;
            Config.AnisotropicFiltering = cJSON_GetObjectItem(root, "iAnisotropicFiltering")->valueint;

            Config.CameraSpeedBase = (real32)cJSON_GetObjectItem(root, "fCameraSpeedBase")->valuedouble;
            Config.CameraSpeedMult = (real32)cJSON_GetObjectItem(root, "fCameraSpeedMult")->valuedouble;
            Config.CameraSpeedAngular = (real32)cJSON_GetObjectItem(root, "fCameraSpeedAngular")->valuedouble;

            cJSON *CameraPositionVector = cJSON_GetObjectItem(root, "vCameraPosition");
            Config.CameraPosition.x = (real32)cJSON_GetArrayItem(CameraPositionVector, 0)->valuedouble;
            Config.CameraPosition.y = (real32)cJSON_GetArrayItem(CameraPositionVector, 1)->valuedouble;
            Config.CameraPosition.z = (real32)cJSON_GetArrayItem(CameraPositionVector, 2)->valuedouble;

            cJSON *CameraTargetVector = cJSON_GetObjectItem(root, "vCameraTarget");
            Config.CameraTarget.x = (real32)cJSON_GetArrayItem(CameraTargetVector, 0)->valuedouble;
            Config.CameraTarget.y = (real32)cJSON_GetArrayItem(CameraTargetVector, 1)->valuedouble;
            Config.CameraTarget.z = (real32)cJSON_GetArrayItem(CameraTargetVector, 2)->valuedouble;
        }
        else
        {
            printf("Error parsing Config File as JSON.\n");
        }
    }
    else
    {
        Config.WindowWidth = 960;
        Config.WindowHeight = 540;
        Config.MSAA = 0;
        Config.FullScreen = false;
        Config.VSync = false;
        Config.FOV = 75.f;
        Config.AnisotropicFiltering = 1;

        Config.CameraSpeedBase = 20.f;
        Config.CameraSpeedMult = 2.f;
        Config.CameraSpeedAngular = 30.f;
        Config.CameraPosition = vec3f(1, 1, 1);
        Config.CameraTarget = vec3f(0, 0, 0);
    }
}

void ProcessKeyboardEvent(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods)
{
    if(Action == GLFW_PRESS)
    {
        FramePressedKeys[Key] = true;
        FrameDownKeys[Key] = true;
        FrameReleasedKeys[Key] = false;
    }
    if(Action == GLFW_RELEASE)
    {
        FramePressedKeys[Key] = false;
        FrameDownKeys[Key] = false;
        FrameReleasedKeys[Key] = true;
    }

    FrameModKeys = Mods;
}

void ProcessMouseButtonEvent(GLFWwindow* Window, int Button, int Action, int Mods)
{
    if(Action == GLFW_PRESS)
    {
        FramePressedMouseButton[Button] = true;
        FrameDownMouseButton[Button] = true;
        FrameReleasedMouseButton[Button] = false;
    }
    if(Action == GLFW_RELEASE)
    {
        FramePressedMouseButton[Button] = false;
        FrameDownMouseButton[Button] = false;
        FrameReleasedMouseButton[Button] = true;
    }

    FrameModKeys = Mods;
}

void ProcessMouseWheelEvent(GLFWwindow *Window, double XOffset, double YOffset)
{
    FrameMouseWheel = (int) YOffset;
}

void ProcessWindowSizeEvent(GLFWwindow *Window, int Width, int Height)
{
    Resized = true;
    ResizeWidth = Width;
    ResizeHeight = Height;
    glViewport(0, 0, Width, Height);
}

void ProcessErrorEvent(int Error, const char* Description)
{
    printf("GLFW Error : %s\n", Description);
}

void WindowResized(game_context *Context)
{
    Resized = false;
    Context->WindowWidth = ResizeWidth;
    Context->WindowHeight = ResizeHeight;
    Context->ProjectionMatrix3D = mat4f::Perspective(Context->FOV, 
            Context->WindowWidth / (real32)Context->WindowHeight, 0.1f, 10000.f);
    Context->ProjectionMatrix2D = mat4f::Ortho(0, Context->WindowWidth, 0, Context->WindowHeight, 0.1f, 1000.f);
}

key_state BuildKeyState(int32 Key)
{
    key_state State = 0;
    State |= (FramePressedKeys[Key] << 0x1);
    State |= (FrameReleasedKeys[Key] << 0x2);
    State |= (FrameDownKeys[Key] << 0x3);

    return State;
}

mouse_state BuildMouseState(int32 MouseButton)
{
    mouse_state State = 0;
    State |= (FramePressedMouseButton[MouseButton] << 0x1);
    State |= (FrameReleasedMouseButton[MouseButton] << 0x2);
    State |= (FrameDownMouseButton[MouseButton] << 0x3);

    return State;
}

void GetFrameInput(game_context *Context, game_input *Input)
{
    memset(FrameReleasedKeys, 0, sizeof(FrameReleasedKeys));
    memset(FramePressedKeys, 0, sizeof(FramePressedKeys));
    memset(FrameReleasedMouseButton, 0, sizeof(FrameReleasedMouseButton));
    memset(FramePressedMouseButton, 0, sizeof(FramePressedMouseButton));

    FrameMouseWheel = 0;

    glfwPollEvents();

    // NOTE - The mouse position can go outside the Window bounds in windowed mode
    // See if this can cause problems in the future.
    real64 MX, MY;
    glfwGetCursorPos(Context->Window, &MX, &MY);
    Input->MousePosX = (int)MX;
    Input->MousePosY = (int)MY;

    if(glfwWindowShouldClose(Context->Window))
    {
        Context->IsRunning = false;
    }

    if(FrameReleasedKeys[GLFW_KEY_ESCAPE])
    {
        Context->IsRunning = false;
    }

    // Get Player controls
    Input->KeyW = BuildKeyState(GLFW_KEY_W);
    Input->KeyA = BuildKeyState(GLFW_KEY_A);
    Input->KeyS = BuildKeyState(GLFW_KEY_S);
    Input->KeyD = BuildKeyState(GLFW_KEY_D);
    Input->KeyR = BuildKeyState(GLFW_KEY_R);
    Input->KeyF = BuildKeyState(GLFW_KEY_F);
    Input->KeyLShift = BuildKeyState(GLFW_KEY_LEFT_SHIFT);
    Input->KeyLCtrl = BuildKeyState(GLFW_KEY_LEFT_CONTROL);
    Input->KeyLAlt = BuildKeyState(GLFW_KEY_LEFT_ALT);
    Input->KeySpace = BuildKeyState(GLFW_KEY_SPACE);
    Input->KeyF1 = BuildKeyState(GLFW_KEY_F1);
    Input->KeyF11 = BuildKeyState(GLFW_KEY_F11);
    Input->KeyNumPlus = BuildKeyState(GLFW_KEY_KP_ADD);
    Input->KeyNumMinus = BuildKeyState(GLFW_KEY_KP_SUBTRACT);

    Input->MouseLeft = BuildMouseState(GLFW_MOUSE_BUTTON_LEFT);
    Input->MouseRight = BuildMouseState(GLFW_MOUSE_BUTTON_RIGHT);

    Input->dTimeFixed = 0.1f; // 100FPS
}

game_context InitContext(game_memory *Memory)
{
    game_context Context = {};
    game_config const &Config = Memory->Config;

    bool GLFWValid = false, GLEWValid = false, ALValid = false;

    GLFWValid = glfwInit();
    if(GLFWValid)
    {
        char WindowName[64];
        snprintf(WindowName, 64, "Radar v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);

        Context.Window = glfwCreateWindow(Config.WindowWidth, Config.WindowHeight, WindowName, NULL, NULL);
        if(Context.Window)
        {
            glfwMakeContextCurrent(Context.Window);

            // TODO - Only in windowed mode for debug
		    glfwSetWindowPos(Context.Window, 800, 400);
            glfwSwapInterval(Config.VSync);

            glfwSetKeyCallback(Context.Window, ProcessKeyboardEvent);
            glfwSetMouseButtonCallback(Context.Window, ProcessMouseButtonEvent);
            glfwSetScrollCallback(Context.Window, ProcessMouseWheelEvent);
            glfwSetWindowSizeCallback(Context.Window, ProcessWindowSizeEvent);
            glfwSetErrorCallback(ProcessErrorEvent);

            GLEWValid = (GLEW_OK == glewInit());
            if(GLEWValid)
            {
                GLubyte const *GLRenderer = glGetString(GL_RENDERER);
                GLubyte const *GLVersion = glGetString(GL_VERSION);
                printf("GL Renderer %s, %s\n", GLRenderer, GLVersion);

                int MaxLayers;
                glGetIntegerv(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS, &MaxLayers);
                printf("GL Max Array Layers : %d\n", MaxLayers);

                Context.WindowWidth = Config.WindowWidth;
                Context.WindowHeight = Config.WindowHeight;
                Context.FOV = Config.FOV;
                Context.ProjectionMatrix3D = mat4f::Perspective(Config.FOV, 
                        Config.WindowWidth / (real32)Config.WindowHeight, 0.1f, 10000.f);
                Context.ProjectionMatrix2D = mat4f::Ortho(0, Config.WindowWidth, 0,Config.WindowHeight, 0.1f, 1000.f);

                Context.WireframeMode = false;
                Context.ClearColor = vec4f(0.01f, 0.19f, 0.31f, 0.f);

                glClearColor(Context.ClearColor.x, Context.ClearColor.y, Context.ClearColor.z, Context.ClearColor.w);

                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glEnable(GL_POINT_SPRITE);
                glEnable(GL_PROGRAM_POINT_SIZE);

                {
                    path TexPath;
                    image Image;

                    MakeRelativePath(TexPath, ExecFullPath, "data/default_diffuse.png");
                    Image = LoadImage(TexPath);
                    Context.DefaultDiffuseTexture = Make2DTexture(&Image, 1);
                    DestroyImage(&Image);

                    MakeRelativePath(TexPath, ExecFullPath, "data/default_normal.png");
                    Image = LoadImage(TexPath);
                    Context.DefaultNormalTexture = Make2DTexture(&Image, 1);
                    DestroyImage(&Image);
#if RADAR_WIN32
                    Context.DefaultFont = LoadFont(Memory, "C:/Windows/Fonts/dejavusansmono.ttf", 24);
#else
                    Context.DefaultFont = LoadFont(Memory, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 24);
#endif
                    Context.DefaultFontTexture = Context.DefaultFont.AtlasTextureID;
                }
            }
            else
            {
                printf("Couldn't initialize GLEW.\n");
            }
        }
        else
        {
            printf("Couldn't create GLFW Window.\n");
        }
    }
    else
    {
        printf("Couldn't init GLFW.\n");
    }

    ALValid = InitAL();

    if(GLFWValid && GLEWValid && ALValid)
    {
        // NOTE - IsRunning might be better elsewhere ?
        Context.IsRunning = true;
        Context.IsValid = true;
    }

    return Context;
}

void DestroyContext(game_context *Context)
{
    DestroyAL();

    if(Context->Window)
    {
        glfwDestroyWindow(Context->Window);
    }
    glfwTerminate();
}

uint32 Program1, Program3D, ProgramSkybox;
uint32 ProgramWater;

void ReloadShaders(game_memory *Memory, path ExecFullPath)
{
    path VSPath;
    path FSPath;
    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/text_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/text_frag.glsl");
    Program1 = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program1);
    SendInt(glGetUniformLocation(Program1, "DiffuseTexture"), 0);

    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/frag.glsl");
    Program3D = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program3D);
    SendInt(glGetUniformLocation(Program3D, "DiffuseTexture"), 0);
    SendInt(glGetUniformLocation(Program3D, "Skybox"), 1);

    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/skybox_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/skybox_frag.glsl");
    ProgramSkybox = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramSkybox);
    SendInt(glGetUniformLocation(ProgramSkybox, "Skybox"), 0);

    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/water_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/water_frag.glsl");
    ProgramWater = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramWater);
    SendInt(glGetUniformLocation(ProgramWater, "Skybox"), 0);

    uiReloadShaders(Memory, ExecFullPath);

    glUseProgram(0);
}

void InitializeFromGame(game_memory *Memory)
{
    // Initialize Water from game Info
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    WaterInitialization(Memory, State, System, State->WaterState);

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

void TestUI(game_memory *Memory)
{
    uiBeginPanel("Title is a pretty long sentence", vec3i(500, 100, 0), vec2i(200, 100), col4f(0,1,0,0.5));
    uiEndPanel();

    uiBeginPanel("Title2", vec3i(600, 150, 1), vec2i(200, 100), col4f(1,0,0,0.5));
    uiEndPanel();
}

int RadarMain(int argc, char **argv)
{
    path DllSrcPath;
    path DllDstPath;

    GetExecutablePath(ExecFullPath);
    MakeRelativePath(DllSrcPath, ExecFullPath, DllName);
    MakeRelativePath(DllDstPath, ExecFullPath, DllDynamicCopyName);

    path ConfigPath;
    MakeRelativePath(ConfigPath, ExecFullPath, "config.json");

    game_memory Memory = InitMemory();
    ParseConfig(&Memory, ConfigPath);
    game_context Context = InitContext(&Memory);
    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_config const &Config = Memory.Config;

    if(Context.IsValid && Memory.IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        uiInit(&Context);

        game_system *System = (game_system*)Memory.PermanentMemPool;
        game_state *State = (game_state*)POOL_OFFSET(Memory.PermanentMemPool, game_system);
/////////////////////////
    // TEMP TESTS
        display_text Texts[ConsoleLogCapacity] = {};

        ALuint AudioBuffer;
        ALuint AudioSource;
        if(TempPrepareSound(&AudioBuffer, &AudioSource))
        {
            alSourcePlay(AudioSource);
        }

        ReloadShaders(&Memory, ExecFullPath);

        glActiveTexture(GL_TEXTURE0);

        // Texture Test
        path TexPath;
        MakeRelativePath(TexPath, ExecFullPath, "data/crate1_diffuse.png");
        image Image = LoadImage(TexPath);
        uint32 Texture1 = Make2DTexture(&Image, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        // Font Test
#if RADAR_WIN32
        font ConsoleFont = LoadFont(&Memory, "C:/Windows/Fonts/dejavusansmono.ttf", 14);
#else
        font ConsoleFont = LoadFont(&Memory, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
#endif

        // Cube Meshes Test
        mesh Cube = MakeUnitCube();
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
        mesh Sphere = MakeUnitSphere();

        int PlaneWidth = 256;
        mesh UnderPlane = Make3DPlane(&Memory, vec2i(PlaneWidth, PlaneWidth), 1, 10);

#if 1 // Skybox 1
        vec3f SunDirection = Normalize(vec3f(0.5, 0.2, 1.0));
#else // Skybox 2
        vec3f SunDirection = Normalize(vec3f(0.7, 1.2, -0.7));
#endif

        // Cubemaps Test
        path CubemapPaths[6];
        {
#if 0
            path CubemapNames[6] = {
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
            };
#else
            path CubemapNames[6] = {
                "data/Skybox/1/right.png",
                "data/Skybox/1/left.png",
                "data/Skybox/1/bottom.png",
                "data/Skybox/1/top.png",
                "data/Skybox/1/back.png",
                "data/Skybox/1/front.png",
            };
#endif
            for(uint32 i = 0; i < 6; ++i)
            {
                MakeRelativePath(CubemapPaths[i], ExecFullPath, CubemapNames[i]);
            }
        }

        mesh SkyboxCube = MakeUnitCube(false);
        uint32 TestCubemap = MakeCubemap(CubemapPaths);
        glBindTexture(GL_TEXTURE_CUBE_MAP, TestCubemap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, TestCubemap);
        glActiveTexture(GL_TEXTURE0);


#if 0
        //mesh ScreenQuad = Make2DQuad(vec2i(-1,1), vec2i(1, -1));
        frame_buffer FBOPass1 = MakeFBO(1, vec2i(Config.WindowWidth, Config.WindowHeight));
        AttachBuffer(&FBOPass1, 0, 1, GL_FLOAT); 
#endif
/////////////////////////
        bool LastDisableMouse = false;

        while(Context.IsRunning)
        {
            game_input Input = {};

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            LastTime = CurrentTime;
            Context.EngineTime += Input.dTime;

            // NOTE - Each frame, clear the Scratch Arena Data
            // TODO - Is this too often ? Maybe let it stay several frames
            ClearArena(&Memory.ScratchArena);

            GetFrameInput(&Context, &Input);        

            uiBeginFrame(&Memory, &Input);


            if(CheckNewDllVersion(&Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Game.GameUpdate(&Memory, &Input);
            if(!Memory.IsInitialized)
            {
                InitializeFromGame(&Memory);
            }

            {
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

                if(LastDisableMouse != State->DisableMouse)
                {
                    if(State->DisableMouse)
                    {
                        glfwSetInputMode(Context.Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    }
                    else
                    {
                        glfwSetInputMode(Context.Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    }

                    LastDisableMouse = State->DisableMouse;
                }
            }

            if(KEY_DOWN(Input.KeyLShift) && KEY_UP(Input.KeyF11))
            {
                ReloadShaders(&Memory, ExecFullPath);
            }

            if(KEY_UP(Input.KeyF1))
            {
                Context.WireframeMode = !Context.WireframeMode;
                glPolygonMode(GL_FRONT_AND_BACK, Context.WireframeMode ? GL_LINE : GL_FILL);
            }

            game_camera &Camera = State->Camera;
            mat4f ViewMatrix = mat4f::LookAt(Camera.Position, Camera.Target, Camera.Up);

            { // NOTE - CUBE DRAWING Test Put somewhere else
                glUseProgram(Program3D);
                // TODO - ProjMatrix updated only when resize happen
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ProjMatrix");
                    SendMat4(Loc, Context.ProjectionMatrix3D);
                    CheckGLError("ProjMatrix Cube");

                    Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }

                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, SunDirection);
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
                    glDrawElements(GL_TRIANGLES, Cube.IndexCount, GL_UNSIGNED_INT, 0);
                }

                // Render sphere at origin
                ModelMatrix.FromTRS(vec3f(0.f), vec3f(0.f), vec3f(1.f));
                SendMat4(Loc, ModelMatrix);
                glBindVertexArray(Sphere.VAO);
                glDrawElements(GL_TRIANGLES, Sphere.IndexCount, GL_UNSIGNED_INT, 0);

                real32 hW = PlaneWidth/2.f;
                ModelMatrix.SetTranslation(vec3f(-hW, -7.f, -hW));
                SendMat4(Loc, ModelMatrix);
                glBindVertexArray(UnderPlane.VAO);
                glDrawElements(GL_TRIANGLES, UnderPlane.IndexCount, GL_UNSIGNED_INT, 0);
            }
#if 1
            UpdateWater(State, System, &Input, State->WaterState, State->WaterStateInterp);
            { // NOTE - Water Rendering Test
                glUseProgram(ProgramWater);
                glDisable(GL_CULL_FACE);
                // TODO - ProjMatrix updated only when resize happen
                {
                    uint32 Loc = glGetUniformLocation(ProgramWater, "ProjMatrix");
                    SendMat4(Loc, Context.ProjectionMatrix3D);
                    CheckGLError("ProjMatrix Cube");

                    Loc = glGetUniformLocation(ProgramWater, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }
                uint32 Loc = glGetUniformLocation(ProgramWater, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(ProgramWater, "CameraPos");
                SendVec3(Loc, State->Camera.Position);
                Loc = glGetUniformLocation(ProgramWater, "SunDirection");
                SendVec3(Loc, SunDirection);

                water_system *WaterSystem = System->WaterSystem;

                real32 hW = PlaneWidth/2.f;

                Loc = glGetUniformLocation(ProgramWater, "ModelMatrix");
                mat4f ModelMatrix;
                glBindVertexArray(WaterSystem->VAO);

                water_beaufort_state *WaterStateA = &WaterSystem->States[State->WaterState];
                water_beaufort_state *WaterStateB = &WaterSystem->States[State->WaterState + 1];
                real32 dWidth = Mix((real32)WaterStateA->Width, (real32)WaterStateB->Width, State->WaterStateInterp);
                
                int Repeat = 3;
                int Middle = (Repeat-1)/2;
                for(int j = 0; j < Repeat; ++j)
                {
                    for(int i = 0; i < Repeat; ++i)
                    {
                        // MIDDLE
                        ModelMatrix.SetTranslation(vec3f(dWidth * (Middle-i), 0.f, dWidth * (Middle-j)));
                        SendMat4(Loc, ModelMatrix);
                        glDrawElements(GL_TRIANGLES, WaterSystem->IndexCount, GL_UNSIGNED_INT, 0);
                    }
                }
                glEnable(GL_CULL_FACE);
            }
#endif

            { // NOTE - Skybox Rendering Test, put somewhere else
                glDisable(GL_CULL_FACE);
                glDepthFunc(GL_LEQUAL);
                CheckGLError("Skybox");

                glUseProgram(ProgramSkybox);
                // TODO - ProjMatrix updated only when resize happen
                {
                    uint32 Loc = glGetUniformLocation(ProgramSkybox, "ProjMatrix");
                    SendMat4(Loc, Context.ProjectionMatrix3D);
                    CheckGLError("ProjMatrix Skybox");

                    // NOTE - remove translation component from the ViewMatrix for the skybox
                    mat4f SkyViewMatrix = ViewMatrix;
                    SkyViewMatrix.SetTranslation(vec3f(0.f));
                    Loc = glGetUniformLocation(ProgramSkybox, "ViewMatrix");
                    SendMat4(Loc, SkyViewMatrix);
                    CheckGLError("ViewMatrix Skybox");
                }
                glBindVertexArray(SkyboxCube.VAO);
                glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, GL_UNSIGNED_INT, 0);

                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);
            }

            { // NOTE - Console Msg Rendering. Put this somewhere else, contained
                console_log *Log = System->ConsoleLog;
                for(uint32 i = 0; i < Log->StringCount; ++i)
                {
                    uint32 RIdx = (Log->ReadIdx + i) % ConsoleLogCapacity;
                    uiMakeText(Log->MsgStack[RIdx], &ConsoleFont, vec3f(10, 10 + i * ConsoleFont.LineGap, 0), 
                            vec4f(0.1f, 0.1f, 0.1f, 1.f), Context.WindowWidth - 10);
                }
            }

            char WStateText[256];
            snprintf(WStateText, 256,"Water State : %d  Water Interpolant : %g", State->WaterState, State->WaterStateInterp);
            uiMakeText(WStateText, &ConsoleFont, vec3f(500, 10, 0), vec4f(0.1, 0.1, 0.1, 1), Context.WindowWidth - 10);
            TestUI(&Memory);
            uiDraw();

            glfwSwapBuffers(Context.Window);
        }

        // TODO - Destroy Console meshes
        DestroyMesh(&Cube);
        DestroyMesh(&SkyboxCube);
        DestroyMesh(&UnderPlane);
        glDeleteTextures(1, &Texture1);
        glDeleteProgram(Program1);
        glDeleteProgram(Program3D);
        glDeleteProgram(ProgramSkybox);
    }

    DestroyMemory(&Memory);
    DestroyContext(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
