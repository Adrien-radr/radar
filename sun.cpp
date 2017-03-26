#include "sun.h"
#include <stdio.h>

real64 Counter = 0.0;

void GameInitialization(game_memory *Memory)
{
    Memory->IsInitialized = true;
}

extern "C" GAMEUPDATE(GameUpdate)
{
    if(!Memory->IsInitialized)
    {
        GameInitialization(Memory);
    }

    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        printf("%g, Mouse: %d,%d\n", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Counter = 0.0;
    }

}
