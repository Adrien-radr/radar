#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AL/al.h"
#include "AL/alc.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "cJSON.h"

#include "radar.h"
#include "sun.h"

// //////// IMPLEMENTATION
#ifdef RADAR_WIN32
#include "radar_win32.cpp"
#else
#ifdef RADAR_UNIX
#include "radar_unix.cpp"
#endif
#endif
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

void MakeRelativePath(char *Dst, char *Path, char* Filename)
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
    State |= (FrameReleasedKeys[Key] << 0x1);
    State |= (FramePressedKeys[Key] << 0x2);
    State |= (FrameDownKeys[Key] << 0x3);

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

    if(FrameReleasedKeys[GLFW_KEY_R])
        Input->KeyReleased = true;

    // Get Player controls
    Input->KeyW = BuildKeyState(GLFW_KEY_W);
    Input->KeyA = BuildKeyState(GLFW_KEY_A);
    Input->KeyS = BuildKeyState(GLFW_KEY_S);
    Input->KeyD = BuildKeyState(GLFW_KEY_D);
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

                //Context.ProjectionMatrix3D = mat4f::Perspective(Config->FOV, 
                        //Config->WindowWidth / (real32)Config->WindowHeight, 0.1f, 1000.f);
                Context.ProjectionMatrix3D = mat4f::Ortho(0, Config->WindowWidth, 0,Config->WindowHeight, 0.1f, 1000.f);

                glClearColor(1.f, 0.f, 1.f, 0.f);

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

void CheckGLError(const char *Mark = "")
{
    uint32 Err = glGetError();
    if(Err != GL_NO_ERROR)
    {
        printf("[%s] GL Error %u\n", Mark, Err);
    }
}

int main()
{
    char DllName[] = "sun.dll";
    char DllDynamicCopyName[] = "sun_temp.dll";

    char ExecFullPath[MAX_PATH];
    char DllSrcPath[MAX_PATH];
    char DllDstPath[MAX_PATH];

    GetExecutablePath(ExecFullPath);
    MakeRelativePath(DllSrcPath, ExecFullPath, DllName);
    MakeRelativePath(DllDstPath, ExecFullPath, DllDynamicCopyName);

    char ConfigPath[MAX_PATH];
    MakeRelativePath(ConfigPath, ExecFullPath, (char*)"config.json");

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
    ALuint AudioBuffer;
    ALuint AudioSource;
    if(TempPrepareSound(&AudioBuffer, &AudioSource))
    {
        alSourcePlay(AudioSource);
    }

    char VSPath[MAX_PATH];
    char FSPath[MAX_PATH];
    MakeRelativePath(VSPath, ExecFullPath, (char*)"data/shaders/2d_billboard_inst_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, (char*)"data/shaders/2d_billboard_inst_frag.glsl");
    uint32 Program1 = BuildShader(VSPath, FSPath);
    glUseProgram(Program1);

    {
        uint32 Loc = glGetUniformLocation(Program1, "ProjMatrix");
        glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) Context.ProjectionMatrix3D);
        CheckGLError();
    }
    

    real32 positions[] = {
        -10.f, 10.f, 0.5f, // topleft
        -10.f, -10.f, 0.5f, // botleft
        10.f, -10.f, 0.5f, // botright
        10.f, 10.f, 0.5f, // topright
    };
    real32 colors[] = {
        1.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f
    };
    uint32 indices[] = { 0, 1, 2, 0, 2, 3 };

    uint32 VAO1 = MakeVertexArrayObject();
    uint32 PosBuffer = AddVertexBufferObject(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(positions), positions);
    uint32 ColBuffer = AddVertexBufferObject(1, 4, GL_FLOAT, GL_STATIC_DRAW, sizeof(colors), colors);
    uint32 IdxBuffer = AddIndexBufferObject(GL_STATIC_DRAW, sizeof(indices), indices);
    glBindVertexArray(0);

/////////////////////////

        while(Context.IsRunning)
        {
            game_input Input = {};

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            // TODO - Fixed Timestep ?
#if 0
            if(Input.dTime < TargetSecondsPerFrame)
            {
                uint32 TimeToSleep = 1000 * (uint32)(TargetSecondsPerFrame - Input.dTime);
                PlatformSleep(TimeToSleep);
                
                CurrentTime = glfwGetTime();
                Input.dTime = CurrentTime - LastTime;
            }
            else
            {
                printf("Missed frame rate, dt=%g\n", Input.dTime);
            }
#endif

            LastTime = CurrentTime;
            Context.EngineTime += Input.dTime;

            GetFrameInput(&Context, &Input);        


            if(CheckNewDllVersion(Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Game.GameUpdate(&Memory, &Input);

            game_state *State = (game_state*)Memory.PermanentMemPool;
            {
                tmp_sound_data *SoundData = State->SoundData;
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

                mat4f ModelMatrix = mat4f::Translation(State->PlayerPosition);
                uint32 Loc = glGetUniformLocation(Program1, "ModelMatrix");
                glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) ModelMatrix);
                CheckGLError();
            }

            glUseProgram(Program1);
            glBindVertexArray(VAO1);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glfwSwapBuffers(Context.Window);
        }

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
