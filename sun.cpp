#include "sun.h"
#include <stdio.h>

real64 Counter = 0.0;

extern "C" GAMEUPDATE(GameUpdate)
{
    Counter += Input->dTime;

    if(Counter > 1.0)
    {
        printf("%g\n", 1.0 / Input->dTime);
        Counter = 0.0;
    }

    if(Input->KeyPressed)
        printf("Key Pressed\n");
    if(Input->KeyReleased)
        printf("Key Released\n");
}
