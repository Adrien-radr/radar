#include <stdio.h>
#include <windows.h>
#include <string.h>

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

struct game_input
{
    real64 dTime;
};

// Note() : expect a MAX_PATH string as Path
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

void ProcessKeyboardEvent(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods)
{
    if(Key == GLFW_KEY_ESCAPE)
    {
        printf("Escape\n");
    }
}

game_context InitContext()
{
    game_context Context = {};

    if(glfwInit())
    {
        char WindowName[64];
        snprintf(WindowName, 64, "Radar v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);

        Context.Window = glfwCreateWindow(960, 540, WindowName, NULL, NULL);
        if(Context.Window)
        {
            glfwMakeContextCurrent(Context.Window);
            glfwSetKeyCallback(Context.Window, ProcessKeyboardEvent);


            // NOTE - IsRunning might be better elsewhere ?
            Context.IsRunning = true;
            Context.IsValid = true;
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

        real32 www;
        while(Context.IsRunning)
        {
            glfwPollEvents();

            // NOTE - Fixed Timestep ?
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
            Sleep(300);
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

            int r = Game.GameUpdate(12, 22);
            printf("%d\n", r);

            if(glfwWindowShouldClose(Context.Window))
            {
                Context.IsRunning = false;
            }

            glfwSwapBuffers(Context.Window);
        }
    }

    DestroyContext(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
