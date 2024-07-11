#include "command-funcs.h"

void command_text(Player *player, lua_State *lua)
{
    (void)player;
    (void)lua;
    printf("command_text\n");
}

void command_test(Player *player, lua_State *lua)
{
    (void)player;
    (void)lua;
    printf("command_test\n");
}
