#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "definitions.h"


namespace game {
    struct state;
}

namespace atmosphere
{
	/// NOTE - Atmospheric scattering engine, inspired by Oskar Elek's Real-Time Spectral Scattering in Large-Scale Natural Participating Media
	/// http://cgg.mff.cuni.cz/~oskar/projects/SCCG2010/Elek2010.pdf
	extern uint32 FmTexture;
	extern uint32 TransmittanceTexture;

	void Init(game::state *State, rf::context *Context);
	void Render(game::state *State, rf::context *Context);
	void ReloadShaders(rf::context *Context);
}

namespace atmosphere2 {
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
