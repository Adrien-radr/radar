#ifndef WATER_H
#define WATER_H

#include "definitions.h"

namespace Water {
    void Initialization(game_memory *Memory, game_state *State, game_system *System, uint32 BeaufortState);
    void Update(game_state *State, water_system *WaterSystem, game_input *Input);
    void Render(game_state *State, water_system *WaterSystem, uint32 Envmap, uint32 Irrmap);
}
#endif
