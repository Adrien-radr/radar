#ifndef WATER_H
#define WATER_H

#include "definitions.h"

struct context;

namespace water {
    struct beaufort_state
    {
        int Width;
        vec2f Direction;
        real32 Amplitude;

        void *OrigPositions;
        void *HTilde0;
        void *HTilde0mk;
    };

    struct system
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

    void Init(memory *Memory, state *State, system *System, uint32 BeaufortState);
    void Update(state *State, water_system *WaterSystem, input *Input);
    void ReloadShaders(memory *Memory, context *Context, water_system *WaterSystem);
    void Render(state *State, water_system *WaterSystem, uint32 Envmap, uint32 GGXLUT);
}
#endif
