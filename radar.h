#ifndef RADAR_H
#define RADAR_H

#include "radar_common.h"

/////////////////////////////////////////////////////////////////////////
// NOTE - This Header must contain what the Radar Platform exposes to the
// Game DLL. Anything that isn't useful for the Game should be elsewhere.
/////////////////////////////////////////////////////////////////////////

real32 static const g_G = 9.807f;

struct game_config
{
    int32  WindowX;
    int32  WindowY;
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

    col4f  DebugFontColor;
};

struct memory_arena
{
    uint8   *BasePtr;   // Start of Arena, in bytes
    uint64  Size;       // Used amount of memory
    uint64  Capacity;   // Total size of arena
};

inline void InitArena(memory_arena *Arena, uint64 Capacity, void *BasePtr)
{
    Arena->BasePtr = (uint8*)BasePtr;
    Arena->Capacity = Capacity;
    Arena->Size = 0;
}

inline void ClearArena(memory_arena *Arena)
{
    // TODO - Do we need to wipe this to 0 each time ? Profile it to see if its 
    // a problem. It seems like best practice
    memset(Arena->BasePtr, 0, sizeof(uint8) * Arena->Capacity);
    Arena->Size = 0;
}

#define PushArenaStruct(Arena, Struct) _PushArenaData((Arena), sizeof(Struct))
#define PushArenaData(Arena, Size) _PushArenaData((Arena), (Size))
inline void *_PushArenaData(memory_arena *Arena, uint64 Size)
{
    Assert(Arena->Size + Size <= Arena->Capacity);
    void *MemoryPtr = Arena->BasePtr + Arena->Size;
    Arena->Size += Size;

    return (void*)MemoryPtr;
}

#define POOL_OFFSET(Pool, Structure) ((uint8*)(Pool) + sizeof(Structure))
// NOTE - This memory is allocated at startup
// Each pool is then mapped according to the needed layout
struct game_memory
{
    path ExecutableFullPath;
    game_config Config;

    // NOTE - For Game State.
    // This is used for a player's game state and should contain everything
    // that is supposed to be saved/load from disk when exiting/entering a
    // savegame.
    // Use cases : hit points, present entities, game timers, etc...
    void *PermanentMemPool;
    uint64 PermanentMemPoolSize;

    // NOTE - For Session Memory
    // Contains everything that is constructed for a whole game session, but,
    // contrary to the PermanentMemPool, is destructed forever when exiting
    // the game (leaving the game, loading another savegame, etc).
    // Use cases : game systems like the ocean, GL resources, Audio buffers, etc)
    void *SessionMemPool;
    uint64 SessionMemPoolSize;
    memory_arena SessionArena;

    // NOTE - For Ephemeral data
    // This can be scratched at any frame, using it for something that will
    // last more than 1 frame is not a good idea.
    // Use cases : temp buffers for quick operations, vertex data before storing
    // in managed GL VBOs or AL audio buffers.
    void *ScratchMemPool;
    uint64 ScratchMemPoolSize;
    memory_arena ScratchArena;

    bool IsValid;
    bool IsInitialized;
    bool IsGameInitialized;
};

// NOTE - Systems fwd declarations
struct water_system;    //water.h
struct tmp_sound_data;  //sound.h
struct ui_frame_stack;  //ui.h
struct console_log;     //ui.h

struct game_system
{
    console_log *ConsoleLog;
    tmp_sound_data *SoundData;
    water_system *WaterSystem;
    ui_frame_stack *UIStack;
    void *DLLStorage;
};

struct game_camera
{
    vec3f  Position;
    vec3f  Target;
    vec3f  Up;
    vec3f  Forward;
    vec3f  Right;

    real32 Phi, Theta;

    real32 LinearSpeed;
    real32 AngularSpeed;
    real32 SpeedMult;

    int    SpeedMode; // -1 : slower, 0 : normal, 1 : faster
    bool   FreeflyMode;
    vec2i  LastMousePos;

    mat4f  ViewMatrix;
};

struct game_state
{
    real64 EngineTime;
    game_camera Camera;
    bool DisableMouse;
    vec3f PlayerPosition;
    vec4f LightColor;
    vec3f LightDirection;

    real64 WaterCounter;
    real32 WaterStateInterp;
    real32 WaterDirection;
    int    WaterState;
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
    real64 dTimeFixed;

    int32  MousePosX;
    int32  MousePosY;

    key_state KeyW;
    key_state KeyA;
    key_state KeyS;
    key_state KeyD;
    key_state KeyR;
    key_state KeyF;
    key_state KeyLShift;
    key_state KeyLCtrl;
    key_state KeyLAlt;
    key_state KeySpace;
    key_state KeyF1;
    key_state KeyF2;
    key_state KeyF11;
    key_state KeyNumPlus;
    key_state KeyNumMinus;
    key_state KeyNumMultiply;
    key_state KeyNumDivide;

    mouse_state MouseLeft;
    mouse_state MouseRight;
};

#endif

