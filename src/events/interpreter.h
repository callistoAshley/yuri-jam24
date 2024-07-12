#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lapi.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "command-funcs.h"
#include "event.h"
#include "../utility/linked-list.h"
#include "../utility/macros.h"

typedef struct
{
    lua_State *lua_state;
    Event *current_event;
} Interpreter;

Interpreter *interpreter_init(void);
