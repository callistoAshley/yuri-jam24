#include "command-funcs.h"

int command_text(Player *player, lua_State *lua)
{
    (void)player;
    (void)lua;
    printf("command_text\n");
}

int command_test(Player *player, lua_State *lua)
{
    (void)player;
    (void)lua;
    printf("command_test\n");
}
