#ifndef WATER_H
#define WATER_H

#include "definitions.h"

void WaterInitialization(game_memory *Memory, game_state *State, game_system *System, uint32 BeaufortState);
void WaterUpdate(game_state *State, water_system *WaterSystem, game_input *Input);
void WaterRender(game_state *State, water_system *WaterSystem, uint32 Envmap, uint32 Irrmap);
#endif
