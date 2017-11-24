#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "definitions.h"

/// NOTE - Atmospheric scattering engine, inspired by Eric Bruneton's Precomputed Atmospheric Scattering
/// https://ebruneton.github.io/precomputed_atmospheric_scattering/

struct game_context;

namespace Atmosphere
{
    static int    const LAMBDA_MIN = 360;
    static int    const LAMBDA_MAX = 830;
    static real32 const LAMBDA_R = 680.0;
    static real32 const LAMBDA_G = 550.0;
    static real32 const LAMBDA_B = 440.0;

    void Init(game_memory *Memory, game_state *State, game_system *System);
    void Update();
    void Render();
    void ReloadShaders(game_memory *Memory, game_context *Context);

    /// Converts a function of wavelength to linear sRGB.
    /// Wavelengths and Spectrum arrays have the same size N
    vec3f ConvertSpectrumToSRGB(real32 *Wavelengths, real32 *Spectrum, int N);
}

#endif