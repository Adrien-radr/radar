#ifndef SUN_H
#define SUN_H

#include "definitions.h"

#define GAMEUPDATE(name) void name(game_memory *Memory, game_input *Input)
typedef GAMEUPDATE(game_update_function);
GAMEUPDATE(GameUpdateStub)
{}

#endif
