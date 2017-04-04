#ifndef RADAR_H
#define RADAR_H

#include "radar_common.h"

#define ConsoleLogStringLen 256
#define ConsoleLogCapacity 8

struct memory_arena
{
    uint8   *BasePtr;   // Start of Arena, in bytes
    uint64  Size;       // Used amount of memory
    uint64  Capacity;   // Total size of arena
};

#define POOL_OFFSET(Pool, Structure) ((uint8*)(Pool) + sizeof(Structure))
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

typedef char console_log_string[ConsoleLogStringLen];
struct console_log
{
    // TODO - Make the length a parameter ? Make the buffer a ring buffer.
    console_log_string MsgStack[ConsoleLogCapacity];
    uint32 WriteIdx;
    uint32 RenderIdx;
    uint32 ReadIdx;
    uint32 StringCount;
};

struct game_system
{
    console_log *ConsoleLog;
    tmp_sound_data *SoundData;
};

struct game_state
{
    vec3f PlayerPosition;
};

typedef uint8 key_state;
#define KEY_HIT(KeyState) ((KeyState >> 0x1) & 1)
#define KEY_UP(KeyState) ((KeyState >> 0x2) & 1)
#define KEY_DOWN(KeyState) ((KeyState >> 0x3) & 1)

// NOTE - Struct passed to the Game
// Contains all frame input each frame needed by game
struct game_input
{
    real64 dTime;

    int32  MousePosX;
    int32  MousePosY;

    bool KeyReleased; // NOTE - TMP for tests

    key_state KeyW;
    key_state KeyA;
    key_state KeyS;
    key_state KeyD;
};

void *ReadFileContents(char *Filename);
void FreeFileContents(void *Contents);
#endif
