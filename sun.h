#ifndef SUN_H
#define SUN_H

#define GAMEUPDATE(name) int name(int a, int b)
typedef GAMEUPDATE(game_update_function);
GAMEUPDATE(GameUpdateStub)
{
    return 0;
}

#endif
