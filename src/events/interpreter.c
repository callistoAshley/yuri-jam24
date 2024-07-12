#include "interpreter.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic" // don't care

extern struct
{
    char *name;
    CommandFn func;
} commands[];

static int free_command_lua(lua_State *lua);

// this function is to be used as a closure for each command
// the first upvalue is the CommandFn associated with this command
// it initializes an instance of the Command struct, fills the args list with heap-allocated equivalents to the lua objects passed as parameters, and returns the Command as a userdata
static int init_command_closure(lua_State *lua)
{
    Command *command;

    command = lua_newuserdatauv(lua, sizeof(Command), 0);
    if (!command) return luaL_error(lua, "init_command_closure: out of memory");
    command->is_userdatum = true;

    command->func = (CommandFn)lua_touserdata(lua, lua_upvalueindex(1)); // gcc warns about this cast, but it should be fine
    if (!command->func) return luaL_error(lua, "init_command_closure: lua_touserdata failure");

    command->args = linked_list_init();
    for (int i = 1; i < lua_gettop(lua); i++)
    {
        // TODO: push the args
        switch (lua_type(lua, i))
        {
            case LUA_TNUMBER:
            {
                double *num = malloc(sizeof(double));
                *num = lua_tonumber(lua, i);
                linked_list_append(command->args, num);
                break;
            }
            case LUA_TBOOLEAN:
            {
                bool *b = malloc(sizeof(bool));
                *b = lua_toboolean(lua, i);
                linked_list_append(command->args, b);
                break;
            }
            case LUA_TSTRING:
            {
                size_t len;
                const char *lua_str = lua_tolstring(lua, i, &len);
                char *str = malloc(len + 1);
                strcpy(str, lua_str);
                linked_list_append(command->args, str);
                break;
            }
            case LUA_TFUNCTION:
            {
                lua_CFunction func = lua_tocfunction(lua, i); // i'm preeetty sure it's safe not to malloc this... if it isn't, i guess i'll find out!
                linked_list_append(command->args, func);
                break;
            }
            default:
            {
                luaL_error(lua, "init_command_closure: bad parameter type at index %d", i);
                break;
            }
        }
    }

    // give the command userdatum a metatable containing the free_command function as its __gc field
    lua_newtable(lua);
    lua_pushcfunction(lua, free_command_lua);
    lua_setfield(lua, -2, "__gc");
    lua_setmetatable(lua, -2);

    return 1;
}

static void free_command(Command *command)
{
    LinkedListNode *node = command->args->first;

    for (int i = 0; i < command->args->len; i++)
    {
        free(node->data);
        node = node->next;
    }

    free(command->args);
    free(command);
}

// this function is to be used as the __gc metafield of a Command userdatum
static int free_command_lua(lua_State *lua)
{
    free_command(lua_touserdata(lua, 1));    
    return 1;
}

Interpreter *interpreter_init(void)
{
    Interpreter *interpreter = calloc(1, sizeof(Interpreter));
    if (!interpreter)
    {
        return NULL;
    }

    interpreter->lua_state = luaL_newstate();
    luaL_openlibs(interpreter->lua_state);

    for (int i = 0;; i++)
    {
        char *name = commands[i].name;
        CommandFn func = commands[i].func;
        if (!name && !func)
            break;
        // this idea was a lot better in my head
        lua_pushcclosure(interpreter->lua_state, init_command_closure, 1);
        lua_pushlightuserdata(interpreter->lua_state, func);
        lua_setupvalue(interpreter->lua_state, -2, -1);
        lua_pop(interpreter->lua_state, 1); // pop the closure
    }

    luaL_dofile(interpreter->lua_state, "assets/events.lua");

    return interpreter;
}

int interpreter_run_event(Interpreter *interpreter, char *name) 
{
    int event_obj_type;
    Event *event;

    (void)event;

    if (interpreter->current_event)
    {
        linked_list_free(interpreter->current_event->commands);
        event_free(interpreter->current_event);
        interpreter->current_event = NULL;
    }

    event_obj_type = lua_getglobal(interpreter->lua_state, name);
    if (event_obj_type != LUA_TTABLE)
    {
        return luaL_error(
            interpreter->lua_state, 
            "interpreter_run_event: the global with the given name has a bad type (expected table, got %s)", 
            lua_typename(interpreter->lua_state, event_obj_type)
        );
    }

    event = event_init(linked_list_init());
    
    // traverse the table and fill the event with its commands
    lua_pushnil(interpreter->lua_state); // initial key
    while (lua_next(interpreter->lua_state, 1))
    {
        if (lua_type(interpreter->lua_state, -1) == LUA_TSTRING)
        {
            // a text command must be manually assembled
            // FIXME: there are too many places where a Command is manually allocated. it would be nice to have an initialization function for them instead
            Command *command = calloc(1, sizeof(Command));
            if (!command) FATAL("ERROR: interpreter_run_event: calloc failure.");
            command->func = command_text;

        }
        else
        {

        }
        lua_pop(interpreter->lua_state, -2); // pop the value
    }
    lua_pop(interpreter->lua_state, -1); // pop the key

    return 1;
}

void interpreter_update(Interpreter *interpreter)
{
    if (interpreter->current_event)
    {
        // TODO: update event
    }
}

void interpreter_free(Interpreter *interpreter) 
{ 
    lua_close(interpreter->lua_state);
    free(interpreter); 
}

#pragma GCC diagnostic pop
