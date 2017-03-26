#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AL/al.h"
#include "AL/alc.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "radar.h"
#include "sun.h"

#ifdef RADAR_WIN32
#include "radar_win32.cpp"
#else
#ifdef RADAR_UNIX
#include "radar_unix.cpp"
#endif
#endif

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

    if(FrameReleasedKeys[GLFW_KEY_ESCAPE] == true)
        Context->IsRunning = false;

    if(FrameReleasedKeys[GLFW_KEY_R])
        Input->KeyReleased = true;
}

game_context InitContext()
{
    game_context Context = {};

    bool GLFWValid = false, GLEWValid = false, ALValid = false;

    GLFWValid = glfwInit();
    if(GLFWValid)
    {
        char WindowName[64];
        snprintf(WindowName, 64, "Radar v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);

        Context.Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowName, NULL, NULL);
        if(Context.Window)
        {
            // TODO - Only in windowed mode for debug
		    glfwSetWindowPos( Context.Window, 800, 400 );
            // TODO - configurable VSYNC
            glfwSwapInterval(0);

            glfwMakeContextCurrent(Context.Window);
            glfwSetKeyCallback(Context.Window, ProcessKeyboardEvent);
            glfwSetMouseButtonCallback(Context.Window, ProcessMouseButtonEvent);
            glfwSetScrollCallback(Context.Window, ProcessMouseWheelEvent);
            glfwSetWindowSizeCallback(Context.Window, ProcessWindowSizeEvent);

            GLEWValid = (GLEW_OK == glewInit());
            if(GLEWValid)
            {
                GLubyte const *GLRenderer = glGetString(GL_RENDERER);
                GLubyte const *GLVersion = glGetString(GL_VERSION);
                printf("GL Renderer %s, %s\n", GLRenderer, GLVersion);

                glClearColor(1.f, 0.f, 1.f, 0.f);

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

        // Clear Error Stack
        alGetError();
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

    // Load Sound
    // TODO - Audio File Loading
#if 0
    ALsizei Size, Freq;
    ALenum Format;
    ALvoid *Data;
    loadWAVFile("test.wav", &Format, &Data, &Size, &Freq, &Loop);
#endif

    // Copy Sound into Buffer


    // Unload Sound

    // Generate Sources
    alGenSources(1, Source);
    if(!CheckALError()) return false;

    // Attach Buffer to Source
    alSourcei(*Source, AL_LOOPING, AL_TRUE);
    //if(!CheckALError()) return false;

    return true;
}

int main()
{
    const char DllName[] = "sun.dll";
    const char DllDynamicCopyName[] = "sun_temp.dll";

    char ExecFullPath[MAX_PATH];
    char DllSrcPath[MAX_PATH];
    char DllDstPath[MAX_PATH];

    GetExecutablePath(ExecFullPath);
    strcpy(DllSrcPath, ExecFullPath);
    strcat(DllSrcPath, DllName);
    strcpy(DllDstPath, ExecFullPath);
    strcat(DllDstPath, DllDynamicCopyName);

    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_context Context = InitContext();
    game_memory Memory = InitMemory();

    bool ValidSound = false;
    ALuint AudioBuffer;
    ALuint AudioSource;
    if(TempPrepareSound(&AudioBuffer, &AudioSource))
    {
        alSourcePlay(AudioSource);
        ValidSound = true;
    }

    if(Context.IsValid && Memory.IsValid && ValidSound)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        while(Context.IsRunning)
        {
            game_input Input = {};
            GetFrameInput(&Context, &Input);        

            // TODO - Fixed Timestep ?
#if 0
            if(Input.dTime < TargetSecondsPerFrame)
            {
                uint32 TimeToSleep = 1000 * (uint32)(TargetSecondsPerFrame - Input.dTime);
                Sleep(TimeToSleep);
                
                CurrentTime = glfwGetTime();
                Input.dTime = TargetSecondsPerFrame;
            }
            else
            {
                printf("Missed frame rate, dt=%g\n", Input.dTime);
            }
#else
            // NOTE - 60FPS HACK. Change that asap
            PlatformSleep(16);
#endif

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            LastTime = CurrentTime;
            Context.EngineTime += Input.dTime;

            if(CheckNewDllVersion(Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }


            if(glfwWindowShouldClose(Context.Window))
            {
                Context.IsRunning = false;
            }

            glClear(GL_COLOR_BUFFER_BIT);

            Game.GameUpdate(&Memory, &Input);

            game_state *State = (game_state*)Memory.ScratchMemPool;
            if(State->ReloadSoundBuffer)
            {
                State->ReloadSoundBuffer = false;
                alSourceStop(AudioSource);
                alSourcei(AudioSource, AL_BUFFER, 0);
                alBufferData(AudioBuffer, AL_FORMAT_MONO16, State->SoundBuffer, State->SoundBufferSize, 48000);
                alSourcei(AudioSource, AL_BUFFER, AudioBuffer);
                alSourcePlay(AudioSource);
                CheckALError();
            }

            glfwSwapBuffers(Context.Window);
        }
    }

    DestroyMemory(&Memory);
    DestroyContext(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
