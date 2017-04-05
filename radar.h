#ifndef RADAR_H
#define RADAR_H

#include "radar_common.h"

/////////////////////////////////////////////////////////////////////////
// NOTE - This Header must contain what the Radar Platform exposes to the
// Game DLL. Anything that isn't useful for the Game should be elsewhere.
/////////////////////////////////////////////////////////////////////////

#define ConsoleLogStringLen 256
#define ConsoleLogCapacity 8

struct game_config
{
    int32  WindowWidth;
    int32  WindowHeight;
    int32  MSAA;
    bool   FullScreen;
    bool   VSync;
    real32 FOV;
    int32  AnisotropicFiltering;

    real32 CameraSpeedBase;
    real32 CameraSpeedMult;
	real32 CameraSpeedAngular;
    vec3f  CameraPosition;
    vec3f  CameraTarget;
};

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
    game_config Config;

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

struct game_camera
{
    vec3f Position;
    vec3f Target;
    vec3f Up;
    vec3f Forward;
    vec3f Right;

    real32 Phi, Theta;

    real32 LinearSpeed;
    real32 AngularSpeed;
    real32 SpeedMult;

    int    SpeedMode; // -1 : slower, 0 : normal, 1 : faster
    bool   FreeflyMode;
    vec2i  LastMousePos;
};

struct game_state
{
    game_camera Camera;
    bool DisableMouse;
    vec3f PlayerPosition;
    vec4f LightColor;
};

typedef uint8 key_state;
#define KEY_HIT(KeyState) ((KeyState >> 0x1) & 1)
#define KEY_UP(KeyState) ((KeyState >> 0x2) & 1)
#define KEY_DOWN(KeyState) ((KeyState >> 0x3) & 1)

typedef uint8 mouse_state;
#define MOUSE_HIT(MouseState) KEY_HIT(MouseState)
#define MOUSE_UP(MouseState) KEY_UP(MouseState)
#define MOUSE_DOWN(MouseState) KEY_DOWN(MouseState)

// NOTE - Struct passed to the Game
// Contains all frame input each frame needed by game
struct game_input
{
    real64 dTime;

    int32  MousePosX;
    int32  MousePosY;

    key_state KeyW;
    key_state KeyA;
    key_state KeyS;
    key_state KeyD;
    key_state KeyLShift;
    key_state KeyLCtrl;
    key_state KeyLAlt;
    key_state KeyF11;

    mouse_state MouseLeft;
    mouse_state MouseRight;
};

// NOTE - Temporary, this should be elswhere.
// The Game shouldn't have Disk IO access at all
void *ReadFileContents(char *Filename, int *FileSize);
void FreeFileContents(void *Contents);

#endif
