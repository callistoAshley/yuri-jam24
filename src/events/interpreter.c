#include "interpreter.h"

extern struct { char *name; CommandFn func; } commands[];

Interpreter *interpreter_init(void)
{
    Interpreter *interpreter = calloc(1, sizeof(Interpreter));
    if (!interpreter)
    {
        return NULL;
    }

    interpreter->lua_state = luaL_newstate();
    luaL_openlibs(interpreter->lua_state);

    for (int i = 0; ; i++)
    {
        char *name = commands[i].name;
        CommandFn func = commands[i].func;
        if (!name && !func) break;
    }

    return interpreter;
}

void interpreter_run_event(Interpreter *interpreter, char *name)
{

}

void interpreter_free(Interpreter *interpreter)
{
    free(interpreter);
}
