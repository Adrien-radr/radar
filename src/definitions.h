#ifndef RADAR_DEFINITIONS_H
#define RADAR_DEFINITIONS_H

#include "rf/rf_common.h"

#define RADAR_MAJOR 0
#define RADAR_MINOR 0
#define RADAR_PATCH 1

// TODO - This is temporarily here, this constant should be part of a physics system
real32 static const g_G = 9.807f;

// NOTE - Systems fwd declarations
struct water_system;    //water.h
struct tmp_sound_data;  //sound.h

struct config
{
    int32   WindowX;
    int32   WindowY;
    int32   WindowWidth;
    int32   WindowHeight;
    int32   MSAA;
    bool    FullScreen;
    bool    VSync;
    real32  FOV;
    real32  NearPlane;
    real32  FarPlane;
    int32   AnisotropicFiltering;

    real32  CameraSpeedBase;
    real32  CameraSpeedMult;
    real32  CameraSpeedAngular;
    vec3f   CameraPosition;
    vec3f   CameraTarget;

    real32  TimeScale; // Ratio for the length of a day. 1.0 is real time. 30.0 is 1 day = 48 minutes
};

// NOTE - This memory is allocated at startup
// Each pool is then mapped according to the needed layout
struct memory
{
    config Config;

    // NOTE - For Game State.
    // This is used for a player's game state and should contain everything
    // that is supposed to be saved/load from disk when exiting/entering a
    // savegame.
    // Use cases : hit points, present entities, game timers, etc...
    void *PermanentMemPool;
    uint64 PermanentMemPoolSize;
    rf::memory_arena PermanentArena;

    // NOTE - For Session Memory
    // Contains everything that is constructed for a whole game session, but,
    // contrary to the PermanentMemPool, is destructed forever when exiting
    // the game (leaving the game, loading another savegame, etc).
    // Use cases : game systems like the ocean, GL resources, Audio buffers, etc)
    void *SessionMemPool;
    uint64 SessionMemPoolSize;
    rf::memory_arena SessionArena;

    // NOTE - For Ephemeral data
    // This can be scratched at any frame, using it for something that will
    // last more than 1 frame is not a good idea.
    // Use cases : temp buffers for quick operations, vertex data before storing
    // in managed GL VBOs or AL audio buffers.
    void *ScratchMemPool;
    uint64 ScratchMemPoolSize;
    rf::memory_arena ScratchArena;

    bool IsValid;
    bool IsInitialized;
    bool IsGameInitialized;
};

struct camera
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
    camera Camera;
    bool DisableMouse;
    vec3f PlayerPosition;
    vec4f LightColor;

    real64 WaterCounter;
    real32 WaterStateInterp;
    real32 WaterDirection;
    int    WaterState;

    vec3f  SunDirection;
    real32 SunSpeed;
};

struct water_beaufort_state
{
    int Width;
    vec2f Direction;
    real32 Amplitude;

    void *OrigPositions;
    void *HTilde0;
    void *HTilde0mk;
};

struct water_system
{
    int static const BeaufortStateCount = 4;
    int static const WaterN = 64;

    size_t VertexDataSize;
    size_t VertexCount;
    real32 *VertexData;
    size_t IndexDataSize;
    uint32 IndexCount;
    uint32 *IndexData;

    water_beaufort_state States[BeaufortStateCount];

    // NOTE - Accessor Pointers, index VertexData
    void *Positions;
    void *Normals;

    complex *hTilde;
    complex *hTildeSlopeX;
    complex *hTildeSlopeZ;
    complex *hTildeDX;
    complex *hTildeDZ;

    // NOTE - FFT system
    int Switch;
    int Log2N;
    complex *FFTC[2];
    complex **FFTW;
    uint32 *Reversed;

    uint32 VAO;
    uint32 VBO[2]; // 0 : idata, 1 : vdata
    uint32 ProgramWater;
};
#endif
