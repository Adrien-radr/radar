#ifndef RADAR_H
#define RADAR_H

#define RADAR_MAJOR 0
#define RADAR_MINOR 0
#define RADAR_PATCH 1

// Platform
#if defined(_WIN32) || defined(_WIN64)
#   define RADAR_WIN32
#elif defined(__unix__) || defined (__unix) || defined(unix)
#   define RADAR_UNIX
#else
#   error "Unknown OS. Only Windows & Linux supported for now."
#endif

typedef float real32;
typedef double real64;

typedef char                int8;
typedef unsigned char       uint8;
typedef short int           int16;
typedef unsigned short int  uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;

#ifdef DEBUG
#ifndef Assert
#define Assert(expr) if(!(expr)) { *(int*)0 = 0; }
#endif
#define DebugPrint(str, ...) printf(str, ##__VA_ARGS__);
#else
#ifndef Assert
#define Assert(expr) 
#endif
#define DebugPrint(str, ...)
#endif

#define Kilobytes(num) (1024*(uint64)(num))
#define Megabytes(num) (1024*Kilobytes(num))
#define Gigabytes(num) (1024*Megabytes(num))

struct memory_arena
{
    uint8   *BasePtr;   // Start of Arena, in bytes
    uint64  Size;       // Used amount of memory
    uint64  Capacity;   // Total size of arena
};

// NOTE - This memory is allocated at startup
// Each pool is then mapped according to the needed layout
struct game_memory
{
    // NOTE - For Game State.
    void *PermanentMemPool;
    uint64 PermanentMemPoolSize;

    // NOTE - For Transient/Reconstructable stuff
    void *ScratchMemPool;
    uint64 ScratchMemPoolSize;
    memory_arena ScratchArena;

    bool IsValid;
    bool IsInitialized;
};

struct tmp_sound_data
{
    bool ReloadSoundBuffer;
    uint32 SoundBufferSize;
    uint16 SoundBuffer[Megabytes(1)];
};

struct game_state
{
    tmp_sound_data *SoundData;
};

// NOTE - Struct passed to the Game
// Contains all frame input each frame needed by game
struct game_input
{
    real64 dTime;

    int32  MousePosX;
    int32  MousePosY;

    bool KeyReleased; // NOTE - TMP for tests
};
#endif
