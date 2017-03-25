#include "sun.h"
#include <stdio.h>

real64 Counter = 0.0;

extern "C" GAMEUPDATE(GameUpdate)
{
    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        printf("%g, Mouse: %d,%d\n", 1.0 / Input->dTime, Input->MousePosX, Input->MousePosY);
        Counter = 0.0;
    }

}
