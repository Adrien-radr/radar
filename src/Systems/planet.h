#pragma once

#include "definitions.h"

namespace game
{
	struct state;
}

namespace planet
{
	void Init(game::state *State, rf::context *Context);
	void Update();
	void Render(game::state *State, rf::context *Context);
	void ReloadShaders(rf::context *Context);
}