#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "radar.h"
#include "sun.h"

struct game_context
{
    GLFWwindow *Window;
    real64 EngineTime;

    bool IsRunning;
    bool IsValid;
};

struct game_code
{
    HMODULE GameDLL;
    FILETIME GameDLLLastWriteTime;
    bool IsValid;

    // DLL Dynamic Entry Points
    game_update_function *GameUpdate;
};

// NOTE : expect a MAX_PATH string as Path
void GetExecutablePath(char *Path)
{
    HMODULE ExecHandle = GetModuleHandleW(NULL);
    GetModuleFileNameA(ExecHandle, Path, MAX_PATH);

    char *LastPos = strrchr(Path, '\\');
    unsigned int NumChar = (unsigned int)(LastPos - Path) + 1;

    // Null-terminate the string at that position before returning
    Path[NumChar] = 0;
}

FILETIME FindLastWriteTime(char *Path)
{
    FILETIME LastWriteTime = {};

    WIN32_FIND_DATA FindData;
    HANDLE H = FindFirstFileA(Path, &FindData);
    if(H != INVALID_HANDLE_VALUE)
    {
        LastWriteTime = FindData.ftLastWriteTime;
        FindClose(H);
    }

    return LastWriteTime;
}

game_code LoadGameCode(char *DllSrcPath, char *DllDstPath)
{
    game_code result = {};

    result.GameUpdate = GameUpdateStub;

    CopyFileA(DllSrcPath, DllDstPath, FALSE);

    result.GameDLL = LoadLibraryA(DllDstPath);

    if(result.GameDLL)
    {
        result.GameDLLLastWriteTime = FindLastWriteTime(DllSrcPath);

        result.GameUpdate = (game_update_function*)GetProcAddress(result.GameDLL, "GameUpdate");
        result.IsValid = (result.GameUpdate != NULL);
    }

    return result;
}

void UnloadGameCode(game_code *Code, char *DuplicateDLL)
{
    if(Code->GameDLL)
    {
        FreeLibrary(Code->GameDLL); 

        if(DuplicateDLL)
        {
            DeleteFileA(DuplicateDLL);
        }
    }

    Code->IsValid = false;
    Code->GameUpdate = GameUpdateStub;
}

bool CheckNewDllVersion(game_code Game, char *DllPath)
{
    FILETIME WriteTime = FindLastWriteTime(DllPath);
    if(CompareFileTime(&Game.GameDLLLastWriteTime, &WriteTime) != 0) {
        Game.GameDLLLastWriteTime = WriteTime;
        return true;
    }

    return false;
}

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
}

game_context InitContext()
{
    game_context Context = {};

    if(glfwInit())
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

            if(GLEW_OK == glewInit())
            {
                GLubyte const *GLRenderer = glGetString(GL_RENDERER);
                GLubyte const *GLVersion = glGetString(GL_VERSION);
                printf("GL Renderer %s, %s\n", GLRenderer, GLVersion);

                glClearColor(1.f, 0.f, 1.f, 0.f);

                // NOTE - IsRunning might be better elsewhere ?
                Context.IsRunning = true;
                Context.IsValid = true;
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
        printf("Couldn't init GLFW3.\n");
    }

    return Context;
}

void DestroyContext(game_context *Context)
{
    if(Context->Window)
    {
        glfwDestroyWindow(Context->Window);
    }
    glfwTerminate();
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
    game_input Input = {};

    if(Context.IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        while(Context.IsRunning)
        {
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
            Sleep(16);
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

            Game.GameUpdate(&Input);

            glfwSwapBuffers(Context.Window);
        }
    }

    DestroyContext(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
