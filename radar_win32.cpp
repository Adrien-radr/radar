#include <windows.h>

static char DllName[] = "sun.dll";
static char DllDynamicCopyName[] = "sun_temp.dll";

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
    game_code Result = {};

    CopyFileA(DllSrcPath, DllDstPath, FALSE);

    Result.GameUpdate = GameUpdateStub;
    Result.GameDLL = LoadLibraryA(DllDstPath);

    if(Result.GameDLL)
    {
        Result.GameDLLLastWriteTime = FindLastWriteTime(DllSrcPath);

        Result.GameUpdate = (game_update_function*)GetProcAddress(Result.GameDLL, "GameUpdate");
        Result.IsValid = (Result.GameUpdate != NULL);
    }

    return Result;
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

bool CheckNewDllVersion(game_code *Game, char *DllPath)
{
    FILETIME WriteTime = FindLastWriteTime(DllPath);
    if(CompareFileTime(&Game->GameDLLLastWriteTime, &WriteTime) != 0) {
        Game->GameDLLLastWriteTime = WriteTime;
        return true;
    }

    return false;
}

void PlatformSleep(DWORD MillisecondsToSleep)
{
    Sleep(MillisecondsToSleep);
}


// NOTE - We just call the platform-agnostic main function here
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
