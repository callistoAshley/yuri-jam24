#include "commands.h"
#include "events/vm.h"
#include "scenes/scene.h"
#include "utility/macros.h"

static Value cmd_printf(VM *vm, u32 arg_count, Resources *resources)
{
    (void)resources;
    if (arg_count != 1)
    {
        FATAL("wrong arity (%d) for print", arg_count)
    }

    Value val = vm_pop(vm);
    switch (val.type)
    {
    case Val_None:
        printf("none\n");
        break;
    case Val_Int:
        printf("%i\n", val.data._int);
        break;
    case Val_Float:
        printf("%f\n", val.data._float);
        break;
    case Val_String:
        printf("%s\n", val.data.string);
        break;
    case Val_True:
        printf("true\n");
        break;
    case Val_False:
        printf("false\n");
        break;
    }
    return NONE_VAL;
}

static Value unimplemented(VM *vm, u32 arg_count, Resources *resources)
{
    (void)vm;
    (void)arg_count;
    (void)resources;
    FATAL("Unimplemented command!");
}

const CommandData COMMANDS[] = {
    [CMD_Printf] = {"printf", cmd_printf},
    [CMD_Text] = {"text", unimplemented},
    [CMD_Wait] = {"wait", unimplemented},
};
