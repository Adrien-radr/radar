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

typedef char int8;
typedef unsigned char uint8;
typedef int int32;
typedef long long int64;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#ifdef DEBUG
#define Assert(expr) if(!(expr)) { *(int*)0 = 0; }
#else
#define Assert(expr) 
#endif

#define Kilobytes(num) (1024*(uint64)(num))
#define Megabytes(num) (1024*Kilobytes(num))
#define Gigabytes(num) (1024*Megabytes(num))

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

    bool IsValid;
    bool IsInitialized;
};

// NOTE - Struct passed to the Game
// Contains all frame input each frame needed by game
struct game_input
{
    real64 dTime;

    int32  MousePosX;
    int32  MousePosY;
};
#endif
