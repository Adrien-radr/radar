#ifndef WATER_H
#define WATER_H

#include "definitions.h"

struct game_context;

namespace Water {
    void Init(game_memory *Memory, game_state *State, game_system *System, uint32 BeaufortState);
    void Update(game_state *State, water_system *WaterSystem, game_input *Input);
    void ReloadShaders(game_memory *Memory, game_context *Context, water_system *WaterSystem);
    void Render(game_state *State, water_system *WaterSystem, uint32 Envmap, uint32 GGXLUT);

    void InitNew(game_memory *Memory, game_state *State, game_system *System);
    void RenderNew(game_state *State, water_system *WaterSystem);
}
#endif
