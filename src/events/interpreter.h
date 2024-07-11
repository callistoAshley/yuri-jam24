#pragma once
#include <stdlib.h>
#include <stdio.h>

#include <lapi.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef struct
{
    lua_State *lua_state;
} Interpreter;

Interpreter *interpreter_init(void);
