#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <lapi.h>
#include <lua.h>

#include "../player.h"

typedef void (*CommandFn)(Player *player, lua_State *lua);

int command_text(Player *player, lua_State *lua);
int command_test(Player *player, lua_State *lua);
