#include "command-funcs.h"

int command_text(lua_State *lua)
{
    (void)lua;
    printf("command_text\n");
    return 1;
}

int command_test(lua_State *lua)
{
    (void)lua;
    printf("command_test\n");
    return 1;
}
