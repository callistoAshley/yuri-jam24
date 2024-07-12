#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <lapi.h>
#include <lua.h>

#include "../player.h"

typedef int (*CommandFn)(lua_State *lua);

int command_text(lua_State *lua);
int command_test(lua_State *lua);
