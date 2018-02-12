#if 0
static path DllName = "sun.dll";
static path DllDynamicCopyName = "sun_temp.dll";
struct game_code
{
    HMODULE GameDLL;
    FILETIME GameDLLLastWriteTime;
    bool IsValid;

    // DLL Dynamic Entry Points
    game_update_function *GameUpdate;
};

FILETIME FindLastWriteTime(path Path)
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

game_code LoadGameCode(path DllSrcPath, path DllDstPath)
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

void UnloadGameCode(game_code *Code, path DuplicateDLL)
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

bool CheckNewDllVersion(game_code *Game, path DllPath)
{
    FILETIME WriteTime = FindLastWriteTime(DllPath);
    if(CompareFileTime(&Game->GameDLLLastWriteTime, &WriteTime) != 0) {
        Game->GameDLLLastWriteTime = WriteTime;
        return true;
    }

    return false;
}
#endif

// NOTE - We just call the platform-agnostic main function here
int main()
{
    return RadarMain(__argc, __argv);
}
