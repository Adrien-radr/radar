#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sun.h"
#include "radar.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RADAR_WIN32
#include "radar_win32.cpp"
#elif RADAR_UNIX
#include "radar_unix.cpp"
#endif

// EXTERNAL
#include "AL/al.h"
#include "AL/alc.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "cJSON.h"


// //////// IMPLEMENTATION
#include "render.cpp"
// ///////////////////////

bool FramePressedKeys[350] = {};
bool FrameReleasedKeys[350] = {};
bool FrameDownKeys[350] = {};

int  FrameModKeys = 0;

bool FramePressedMouseButton[8] = {};
bool FrameDownMouseButton[8] = {};
bool FrameReleasedMouseButton[8] = {};

int  FrameMouseWheel = 0;

int WindowWidth = 960;
int WindowHeight = 540;

struct game_context
{
    GLFWwindow *Window;
    real64 EngineTime;

    mat4f ProjectionMatrix3D;
    mat4f ProjectionMatrix2D;

    bool IsRunning;
    bool IsValid;
};

game_memory InitMemory()
{
    game_memory Memory = {};

    Memory.PermanentMemPoolSize = Megabytes(64);
    Memory.ScratchMemPoolSize = Megabytes(512);

    Memory.PermanentMemPool = calloc(1, Memory.PermanentMemPoolSize);
    Memory.ScratchMemPool = calloc(1, Memory.ScratchMemPoolSize);

    Memory.IsValid = Memory.PermanentMemPool && Memory.ScratchMemPool;
    Memory.IsInitialized = false;

    return Memory;
}

void DestroyMemory(game_memory *Memory)
{
    if(Memory->IsValid)
    {
        free(Memory->PermanentMemPool);
        free(Memory->ScratchMemPool);
        Memory->PermanentMemPoolSize = 0;
        Memory->ScratchMemPoolSize = 0;
        Memory->IsValid = false;
        Memory->IsInitialized = false;
    }
}

void MakeRelativePath(char *Dst, char *Path, char const *Filename)
{
    strncpy(Dst, Path, MAX_PATH);
    strncat(Dst, Filename, MAX_PATH);
}

void *ReadFileContents(char *Filename)
{
    char *Contents = NULL;
    FILE *fp = fopen(Filename, "rb");

    if(fp)
    {
        if(0 == fseek(fp, 0, SEEK_END))
        {
            int32 Size = ftell(fp);
            rewind(fp);
            Contents = (char*)malloc(Size+1);
            fread(Contents, Size, 1, fp);
            Contents[Size] = 0;
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

void FreeFileContents(void *Contents)
{
    free(Contents);
}

struct game_config
{
    int32  WindowWidth;
    int32  WindowHeight;
    int32  MSAA;
    bool   FullScreen;
    bool   VSync;
    real32 FOV;
    int32  AnisotropicFiltering;

    real32 CameraSpeedBase;
    real32 CameraSpeedMult;
	real32 CameraSpeedRotation;
    vec3f  CameraPosition;
    vec3f  CameraTarget;
};

game_config ParseConfig(char *ConfigPath)
{
    game_config Config = {};

    void *Content = ReadFileContents(ConfigPath);
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
            Config.CameraSpeedRotation = (real32)cJSON_GetObjectItem(root, "fCameraSpeedRotation")->valuedouble;

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
        FreeFileContents(Content);
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
        Config.CameraSpeedRotation = 30.f;
        Config.CameraPosition = vec3f(1, 1, 1);
        Config.CameraTarget = vec3f(0, 0, 0);
    }

    return Config;
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
    WindowWidth = Width;
    WindowHeight = Height;
    glViewport(0, 0, Width, Height);
}

void ProcessErrorEvent(int Error, const char* Description)
{
    printf("GLFW Error : %s\n", Description);
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
    Input->KeyLShift = BuildKeyState(GLFW_KEY_LEFT_SHIFT);
    Input->KeyLCtrl = BuildKeyState(GLFW_KEY_LEFT_CONTROL);
    Input->KeyLAlt = BuildKeyState(GLFW_KEY_LEFT_ALT);

    Input->MouseLeft = BuildMouseState(GLFW_MOUSE_BUTTON_LEFT);
    Input->MouseRight = BuildMouseState(GLFW_MOUSE_BUTTON_RIGHT);
}

game_context InitContext(game_config *Config)
{
    game_context Context = {};

    bool GLFWValid = false, GLEWValid = false, ALValid = false;

    GLFWValid = glfwInit();
    if(GLFWValid)
    {
        char WindowName[64];
        snprintf(WindowName, 64, "Radar v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);

        Context.Window = glfwCreateWindow(Config->WindowWidth, Config->WindowHeight, WindowName, NULL, NULL);
        if(Context.Window)
        {
            glfwMakeContextCurrent(Context.Window);

            // TODO - Only in windowed mode for debug
		    glfwSetWindowPos(Context.Window, 800, 400);
            glfwSwapInterval(Config->VSync);

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

                Context.ProjectionMatrix3D = mat4f::Perspective(Config->FOV, 
                        Config->WindowWidth / (real32)Config->WindowHeight, 0.1f, 1000.f);
                Context.ProjectionMatrix2D = mat4f::Ortho(0, Config->WindowWidth, 0,Config->WindowHeight, 0.1f, 1000.f);

                glClearColor(0.2f, 0.3f, 0.7f, 0.f);

                glEnable( GL_CULL_FACE );
                glCullFace( GL_BACK );
                glFrontFace( GL_CCW );

                glEnable( GL_DEPTH_TEST );
                glDepthFunc( GL_LESS );

                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
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

    ALCdevice *ALDevice = alcOpenDevice(NULL);

    ALValid = (NULL != ALDevice);
    if(ALValid)
    {
        ALCcontext *ALContext = alcCreateContext(ALDevice, NULL);
        alcMakeContextCurrent(ALContext);
        alGetError(); // Clear Error Stack
    }
    else
    {
        printf("Couldn't init OpenAL.\n");
    }

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
    ALCcontext *ALContext = alcGetCurrentContext();
    if(ALContext)
    {
        ALCdevice *ALDevice = alcGetContextsDevice(ALContext);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(ALContext);
        alcCloseDevice(ALDevice);
    }

    if(Context->Window)
    {
        glfwDestroyWindow(Context->Window);
    }
    glfwTerminate();
}

bool CheckALError()
{
    ALenum Error = alGetError();
    if(Error != AL_NO_ERROR)
    {
        char ErrorMsg[128];
        switch(Error)
        {
        case AL_INVALID_NAME:
        {
            snprintf(ErrorMsg, 128, "Bad Name ID passed to AL."); 
        }break;
        case AL_INVALID_ENUM:
        {
            snprintf(ErrorMsg, 128, "Invalid Enum passed to AL."); 
        }break;
        case AL_INVALID_VALUE:
        {
            snprintf(ErrorMsg, 128, "Invalid Value passed to AL."); 
        }break;
        case AL_INVALID_OPERATION:
        {
            snprintf(ErrorMsg, 128, "Invalid Operation requested to AL."); 
        }break;
        case AL_OUT_OF_MEMORY:
        {
            snprintf(ErrorMsg, 128, "Out of Memory."); 
        }break;
        }

        printf("OpenAL Error : %s\n", ErrorMsg);
        return false;
    }
    return true;
}

bool TempPrepareSound(ALuint *Buffer, ALuint *Source)
{
    // Generate Buffers
    alGenBuffers(1, Buffer);
    if(!CheckALError()) return false;

    // Generate Sources
    alGenSources(1, Source);
    if(!CheckALError()) return false;

    // Attach Buffer to Source
    alSourcei(*Source, AL_LOOPING, AL_TRUE);
    if(!CheckALError()) return false;

    return true;
}

void DestroyDisplayText(display_text *Text)
{
    Text->IndexCount = 0;
    glDeleteBuffers(3, Text->VBO);
    glDeleteVertexArrays(1, &Text->VAO);
}

display_text MakeDisplayText(font *Font, char const *Msg, int MaxPixelWidth, vec4f Color, real32 Scale = 1.0f)
{
    display_text Text = {};

    uint32 MsgLength = strlen(Msg);
    uint32 VertexCount = MsgLength * 4;
    uint32 IndexCount = MsgLength * 6;

    uint32 PS = 4 * 3;
    uint32 TS = 4 * 2;

    real32 *Positions = (real32*)alloca(3 * VertexCount * sizeof(real32));
    real32 *Texcoords = (real32*)alloca(2 * VertexCount * sizeof(real32));
    uint32 *Indices = (uint32*)alloca(IndexCount * sizeof(uint32));

    int X = 0, Y = 0;
    for(uint32 i = 0; i < MsgLength; ++i)
    {
        uint8 AsciiIdx = Msg[i] - 32; // 32 is the 1st Ascii idx
        glyph &Glyph = Font->Glyphs[AsciiIdx];

        // Modify DisplayWidth to always be at least the length of each character
        if(MaxPixelWidth < Glyph.CW)
        {
            MaxPixelWidth = Glyph.CW;
        }

        if(Msg[i] == '\n')
        {
            X = 0;
            Y -= Font->LineGap;
            AsciiIdx = Msg[++i] - 32;
            Glyph = Font->Glyphs[AsciiIdx];
            IndexCount -= 6;
        }

        if((X + Glyph.CW) >= MaxPixelWidth)
        {
            X = 0;
            Y -= Font->LineGap;
        }

        // position (TL, BL, BR, TR)
        real32 BaseX = (real32)(X + Glyph.X);
        real32 BaseY = (real32)(Y - Font->Ascent - Glyph.Y);
        Positions[i*PS+0+0] = Scale*(BaseX);              Positions[i*PS+0+1] = Scale*(BaseY);            Positions[i*PS+0+2] = 0;
        Positions[i*PS+3+0] = Scale*(BaseX);              Positions[i*PS+3+1] = Scale*(BaseY - Glyph.CH); Positions[i*PS+3+2] = 0;
        Positions[i*PS+6+0] = Scale*(BaseX + Glyph.CW);   Positions[i*PS+6+1] = Scale*(BaseY - Glyph.CH); Positions[i*PS+6+2] = 0;
        Positions[i*PS+9+0] = Scale*(BaseX + Glyph.CW);   Positions[i*PS+9+1] = Scale*(BaseY);            Positions[i*PS+9+2] = 0;

        // texcoords
        Texcoords[i*TS+0+0] = Glyph.TexX0; Texcoords[i*TS+0+1] = Glyph.TexY0;
        Texcoords[i*TS+2+0] = Glyph.TexX0; Texcoords[i*TS+2+1] = Glyph.TexY1;
        Texcoords[i*TS+4+0] = Glyph.TexX1; Texcoords[i*TS+4+1] = Glyph.TexY1;
        Texcoords[i*TS+6+0] = Glyph.TexX1; Texcoords[i*TS+6+1] = Glyph.TexY0;

        Indices[i*6+0] = i*4+0;Indices[i*6+1] = i*4+1;Indices[i*6+2] = i*4+2;
        Indices[i*6+3] = i*4+0;Indices[i*6+4] = i*4+2;Indices[i*6+5] = i*4+3;

        X += Glyph.AdvX;
    }

    Text.VAO = MakeVertexArrayObject();
    Text.VBO[0] = AddVertexBufferObject(0, 3, GL_FLOAT, GL_STATIC_DRAW, 3 * VertexCount * sizeof(real32), Positions);
    Text.VBO[1] = AddVertexBufferObject(1, 2, GL_FLOAT, GL_STATIC_DRAW, 2 * VertexCount * sizeof(real32), Texcoords);
    Text.VBO[2] = AddIndexBufferObject(GL_STATIC_DRAW, IndexCount * sizeof(uint32), Indices);
    Text.IndexCount = IndexCount;
    glBindVertexArray(0);

    Text.Texture = Font->AtlasTextureID;
    Text.Color = Color;

    return Text;
}

int RadarMain(int argc, char **argv)
{
    path ExecFullPath;
    path DllSrcPath;
    path DllDstPath;

    GetExecutablePath(ExecFullPath);
    MakeRelativePath(DllSrcPath, ExecFullPath, DllName);
    MakeRelativePath(DllDstPath, ExecFullPath, DllDynamicCopyName);

    path ConfigPath;
    MakeRelativePath(ConfigPath, ExecFullPath, "config.json");

    game_config Config = ParseConfig(ConfigPath);
    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_context Context = InitContext(&Config);
    game_memory Memory = InitMemory();

    if(Context.IsValid && Memory.IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

/////////////////////////
    // TEMP
        display_text Texts[ConsoleLogCapacity] = {};

        ALuint AudioBuffer;
        ALuint AudioSource;
        if(TempPrepareSound(&AudioBuffer, &AudioSource))
        {
            alSourcePlay(AudioSource);
        }

        path VSPath;
        path FSPath;
        MakeRelativePath(VSPath, ExecFullPath, "data/shaders/text_vert.glsl");
        MakeRelativePath(FSPath, ExecFullPath, "data/shaders/text_frag.glsl");
        uint32 Program1 = BuildShader(VSPath, FSPath);
        glUseProgram(Program1);
        glUniform1i(glGetUniformLocation(Program1, "DiffuseTexture"), 0);

        MakeRelativePath(VSPath, ExecFullPath, "data/shaders/vert.glsl");
        MakeRelativePath(FSPath, ExecFullPath, "data/shaders/frag.glsl");
        uint32 Program3D = BuildShader(VSPath, FSPath);
        glUseProgram(Program3D);
        glUniform1i(glGetUniformLocation(Program3D, "DiffuseTexture"), 0);



        real32 positions[] = {
            -100.f, 100.f, 0.5f, // topleft
            -100.f, -100.f, 0.5f, // botleft
            100.f, -100.f, 0.5f, // botright
            100.f, 100.f, 0.5f, // topright
        };
        real32 colors[] = {
            1.f, 1.f, 1.f, 1.f,
            0.f, 1.f, 0.f, 1.f,
            0.f, 1.f, 1.f, 1.f,
            1.f, 0.f, 0.f, 1.f
        };
        real32 texcoords[] = { // NOTE - inverted Y coordinate ! Streamline this somewhere
            0.f, 0.f,
            0.f, 1.f,
            1.f, 1.f,
            1.f, 0.f
        };
        uint32 indices[] = { 0, 1, 2, 0, 2, 3 };

        uint32 VAO1 = MakeVertexArrayObject();
        uint32 PosBuffer = AddVertexBufferObject(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(positions), positions);
        uint32 TexBuffer = AddVertexBufferObject(1, 2, GL_FLOAT, GL_STATIC_DRAW, sizeof(texcoords), texcoords);
        uint32 ColBuffer = AddVertexBufferObject(2, 4, GL_FLOAT, GL_STATIC_DRAW, sizeof(colors), colors);
        uint32 IdxBuffer = AddIndexBufferObject(GL_STATIC_DRAW, sizeof(indices), indices);
        glBindVertexArray(0);

        // Texture creation
        path TexPath;
        MakeRelativePath(TexPath, ExecFullPath, "data/crate1_diffuse.png");
        image Image = LoadImage(TexPath);
        uint32 Texture1 = Make2DTexture(&Image, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        // Load Font Char
#if RADAR_WIN32
        font Font = LoadFont("C:/Windows/Fonts/dejavusansmono.ttf", 24);
        font ConsoleFont = LoadFont("C:/Windows/Fonts/dejavusansmono.ttf", 14);
        //font Font = LoadFont((char*)"C:/Windows/Fonts/arial.ttf", 24);
#else
        font Font = LoadFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 24);
        font ConsoleFont = LoadFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
#endif

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

/////////////////////////
        bool LastDisableMouse = false;

        while(Context.IsRunning)
        {
            game_input Input = {};

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            LastTime = CurrentTime;
            Context.EngineTime += Input.dTime;

            GetFrameInput(&Context, &Input);        


            if(CheckNewDllVersion(&Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Game.GameUpdate(&Memory, &Input);

            game_system *System = (game_system*)Memory.PermanentMemPool;
            game_state *State = (game_state*)POOL_OFFSET(Memory.PermanentMemPool, game_system);
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

            { // NOTE - CUBE DRAWING
                glUseProgram(Program3D);
                // TODO - ProjMatrix updated only when resize happen
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ProjMatrix");
                    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) Context.ProjectionMatrix3D);
                    CheckGLError("ProjMatrix");

                    game_camera &Camera = State->Camera;
                    mat4f ViewMatrix = mat4f::LookAt(Camera.Position, Camera.Target, Camera.Up);
                    Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) ViewMatrix);
                    CheckGLError("ViewMatrix");

                }
                glBindVertexArray(Cube.VAO);
                glBindTexture(GL_TEXTURE_2D, Texture1);
                mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
                uint32 Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                for(uint32 i = 0; i < 5; ++i)
                {
                    ModelMatrix.FromTRS(CubePos[i], CubeRot[i], vec3f(1.f));
                    //ModelMatrix.SetTranslation(CubePos[i]);
                    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) ModelMatrix);
                    CheckGLError("ModelMatrix");
                    glDrawElements(GL_TRIANGLES, Cube.IndexCount, GL_UNSIGNED_INT, 0);
                }
            }


            { // NOTE - Console Msg Rendering. Put this somewhere else, contained
                glUseProgram(Program1);
                // TODO - ProjMatrix updated only when resize happen
                {
                    uint32 Loc = glGetUniformLocation(Program1, "ProjMatrix");
                    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) Context.ProjectionMatrix2D);
                    CheckGLError("ProjMatrix");
                }
                uint32 Loc = glGetUniformLocation(Program1, "ModelMatrix");
                mat4f ConsoleModelMatrix;
                console_log *Log = System->ConsoleLog;
                for(uint32 i = 0; i < Log->StringCount; ++i)
                {
                    uint32 RIdx = (Log->ReadIdx + i) % ConsoleLogCapacity;
                    if(Log->RenderIdx != Log->WriteIdx)
                    {
                        // NOTE - If already created in previous pass : delete it first to free GL resources
                        if(Texts[Log->RenderIdx].IndexCount > 0)
                        {
                            DestroyDisplayText(&Texts[Log->RenderIdx]);
                        }
                        Texts[Log->RenderIdx] = MakeDisplayText(&ConsoleFont, Log->MsgStack[Log->RenderIdx], 
                                Config.WindowWidth - 20, vec4f(0.8f, 0.8f, 0.8f, 1.f), 1.f);
                        Log->RenderIdx = (Log->RenderIdx + 1) % ConsoleLogCapacity;
                    }

                    ConsoleModelMatrix.SetTranslation(vec3f(10, Config.WindowHeight - 10 - i * ConsoleFont.LineGap, 0));
                    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) ConsoleModelMatrix);
                    glUniform4fv(glGetUniformLocation(Program1, "TextColor"), 1, (GLfloat*)Texts[RIdx].Color);

                    glBindVertexArray(Texts[RIdx].VAO);
                    glBindTexture(GL_TEXTURE_2D, Texts[RIdx].Texture);
                    glDrawElements(GL_TRIANGLES, Texts[RIdx].IndexCount, GL_UNSIGNED_INT, 0);
                }
            }

            glfwSwapBuffers(Context.Window);
        }

        // TODO - Destroy Console meshes
        DestroyMesh(&Cube);
        DestroyFont(&Font);
        DestroyFont(&ConsoleFont);
        glDeleteTextures(1, &Texture1);
        glDeleteBuffers(1, &PosBuffer);
        glDeleteBuffers(1, &ColBuffer);
        glDeleteBuffers(1, &IdxBuffer);
        glDeleteVertexArrays(1, &VAO1);
        glDeleteProgram(Program1);
    }

    DestroyMemory(&Memory);
    DestroyContext(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
