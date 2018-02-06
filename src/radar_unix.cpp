#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>

static path DllName = "sun.so";
static path DllDynamicCopyName = "sun_temp.so";

#if 0
struct game_code
{
    void *GameDLL;
    time_t GameDLLLastWriteTime;
    bool IsValid;

    // DLL Dynamic Entry Points
    game_update_function *GameUpdate;
};

time_t FindLastWriteTime(path Path)
{
    struct stat Info;
    stat(Path, &Info);
    return Info.st_mtime;
}

game_code LoadGameCode(path DllSrcPath, path DllDstPath)
{
    game_code Result = {};

    CopyFile(DllSrcPath, DllDstPath);

    Result.GameUpdate = GameUpdateStub;
    Result.GameDLL = dlopen(DllDstPath, RTLD_NOW);

    if(Result.GameDLL)
    {
        Result.GameDLLLastWriteTime = FindLastWriteTime(DllSrcPath);
        Result.GameUpdate = (game_update_function*)dlsym(Result.GameDLL, "GameUpdate");
        Result.IsValid = (Result.GameUpdate != NULL);
    }

    return Result;

}

void UnloadGameCode(game_code *Code, path DuplicateDLL)
{
    if(Code->GameDLL)
    {
        dlclose(Code->GameDLL);
        Code->GameDLL = 0;

        if(DuplicateDLL)
        {
            unlink(DuplicateDLL);
        }
    }

    Code->IsValid = false;
    Code->GameUpdate = GameUpdateStub;
}

bool CheckNewDllVersion(game_code *Game, path DllPath)
{
    time_t WriteTime = FindLastWriteTime(DllPath);
    if(WriteTime != Game->GameDLLLastWriteTime)
    {
        Game->GameDLLLastWriteTime = WriteTime;
        return true;
    }

    return false;
}
#endif

int main(int argc, char **argv)
{
    return RadarMain(argc, argv);
}
