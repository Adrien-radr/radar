#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "GLFW/glfw3.h"

#include "radar.h"
#include "sun.h"

struct game_context
{
    GLFWwindow *Window;

    bool IsValid;
};

struct game_code
{
    HMODULE GameDLL;
    FILETIME GameDLLLastWriteTime;
    bool IsValid;

    // DLL Dynamic Entry Points
    game_func *func1;
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

    result.func1 = Func1Stub;

    CopyFileA(DllSrcPath, DllDstPath, FALSE);

    result.GameDLL = LoadLibraryA(DllDstPath);

    if(result.GameDLL)
    {
        result.GameDLLLastWriteTime = FindLastWriteTime(DllSrcPath);

        result.func1 = (game_func*)GetProcAddress(result.GameDLL, "GameFunc");
        result.IsValid = (result.func1 != NULL);
    }

    return result;
}

void UnloadGameCode(game_code *code)
{
    if(code->GameDLL)
    {
        FreeLibrary(code->GameDLL); 
    }

    code->IsValid = false;
    code->func1 = Func1Stub;
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

    if(Context.IsValid)
    {
        while(true)
        {
            if(CheckNewDllVersion(Game, DllSrcPath))
            {
                UnloadGameCode(&Game);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            int r = Game.func1(12, 22);
            printf("%d\n", r);

            Sleep(1000);
        }
    }

    DestroyContext(&Context);
    UnloadGameCode(&Game);

    return 0;
}
