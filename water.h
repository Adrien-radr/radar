#ifndef WATER_H
#define WATER_H

#include "radar.h"

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

void WaterInitialization(game_memory *Memory, game_state *State, game_system *System, uint32 BeaufortState);
void WaterUpdate(game_state *State, water_system *WaterSystem, game_input *Input);
void WaterRender(game_state *State, water_system *WaterSystem, uint32 Envmap, uint32 Irrmap);
#endif
