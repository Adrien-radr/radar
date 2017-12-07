#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>

static path DllName = "sun.so";
static path DllDynamicCopyName = "sun_temp.so";

struct game_code
{
    void *GameDLL;
    time_t GameDLLLastWriteTime;
    bool IsValid;

    // DLL Dynamic Entry Points
    game_update_function *GameUpdate;
};

// NOTE : expect a MAX_PATH string as Path
bool GetExecutablePath(path Path)
{
    struct stat Info;
    path StatProc;

    pid_t pid = getpid();
    snprintf(StatProc, MAX_PATH, "/proc/%d/exe", pid);
    ssize_t BytesRead = readlink(StatProc, Path, MAX_PATH);
    if(-1 == BytesRead)
    {
        printf("Fatal Error : Can't query Executable path.\n");
        return false;
    }

    Path[BytesRead] = 0;

    char *LastPos = strrchr(Path, '/');
    unsigned int NumChar = (unsigned int)(LastPos - Path) + 1;

    // Null-terminate the string at that position before returning
    Path[NumChar] = 0;

    return true;
}

time_t FindLastWriteTime(path Path)
{
    struct stat Info;
    stat(Path, &Info);
    return Info.st_mtime;
}

static void CopyFile(path const Src, path const Dst)
{
    // NOTE - No CopyFile on Linux : open Src, read it and copy it in Dst
    int SFD = open(Src, O_RDONLY);
    int DFD = open(Dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    char FileBuf[Kilobytes(4)];

    // Read until we can't anymore
    while(1)
    {
        ssize_t Read = read(SFD, FileBuf, sizeof(FileBuf));
        if(!Read)
        {
            break;
        }
        ssize_t Written = write(DFD, FileBuf, Read);
        if(Read != Written)
        {
            printf("Copy File Error [%s].\n", Dst);
        }
    }

    close(SFD);
    close(DFD);
}

void DiskFileCopy(path const DstPath, path const SrcPath)
{
    CopyFile(SrcPath, DstPath);
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

void PlatformSleep(uint32 MillisecondsToSleep)
{
    struct timespec TS;
    TS.tv_sec = MillisecondsToSleep / 1000;
    TS.tv_nsec = (MillisecondsToSleep % 1000) * 1000000;
    nanosleep(&TS, NULL);
}

int main(int argc, char **argv)
{
    return RadarMain(argc, argv);
}
