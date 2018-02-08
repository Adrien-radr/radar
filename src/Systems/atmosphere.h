#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "definitions.h"

/// NOTE - Atmospheric scattering engine, inspired by Eric Bruneton's Precomputed Atmospheric Scattering
/// https://ebruneton.github.io/precomputed_atmospheric_scattering/

namespace game {
    struct state;
}

namespace atmosphere {
    static int    const LAMBDA_MIN = 360;
    static int    const LAMBDA_MAX = 830;
    static real32 const LAMBDA_R = 680.0;
    static real32 const LAMBDA_G = 550.0;
    static real32 const LAMBDA_B = 440.0;
    extern uint32 TransmittanceTexture;
    extern uint32 IrradianceTexture;
    extern uint32 ScatteringTexture;

    // Conversion factor between watts and lumens
    static real32 const MAX_LUMINOUS_EFFICACY = 683.f;

    void Init(game::state *State, rf::context *Context);
    void Update();
    void Render(game::state *State, rf::context *Context);
    void ReloadShaders(rf::context *Context);

    /// Converts a function of wavelength to linear sRGB.
    /// Wavelengths and Spectrum arrays have the same size N
    vec3f ConvertSpectrumToSRGB(real32 *Wavelengths, real32 *Spectrum, int N);
}

#endif
