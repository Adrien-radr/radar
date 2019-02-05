#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "definitions.h"


namespace game {
    struct state;
}

namespace atmosphere {
	
	enum radiance_mode
	{
		RGB,
		SRGB,
		FULL
	};

	/// NOTE - Atmospheric scattering engine, inspired by Eric Bruneton's Precomputed Atmospheric Scattering
	/// https://ebruneton.github.io/precomputed_atmospheric_scattering/
    extern uint32 TransmittanceTexture;
    extern uint32 IrradianceTexture;
    extern uint32 ScatteringTexture;
	
    void Init(game::state *State, rf::context *Context);
    void Update();
    void Render(game::state *State, rf::context *Context);
    void ReloadShaders(rf::context *Context);

}

#endif
