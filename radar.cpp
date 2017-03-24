#include "sun.h"
#include <stdio.h>
#include <windows.h>

struct game_code
{
    HMODULE GameDLL;
    game_func *func1;
};

game_code LoadGameCode()
{
    game_code result = {};

    result.GameDLL = LoadLibrary("sun.dll");
    if(result.GameDLL)
    {
        result.func1 = (game_func*)GetProcAddress(result.GameDLL, "GameFunc");
    }

    return result;
}

void UnloadGameCode(game_code *code)
{
   FreeLibrary(code->GameDLL); 
}

int main()
{
    game_code game = LoadGameCode();
    int r = game.func1(12, 22);
    printf("%d\n", r);

    UnloadGameCode(&game);

    return 0;
}
