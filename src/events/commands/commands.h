#pragma once

#include "events/value.h"
#include "command.h"
#include "events/vm.h"
#include "resources.h"

// commands do not need to fill in the out pointer unless they are returning
// values.
//
// NOTE
// if a command returns `true`, the vm will yield execution.
// the next time the vm starts executing, it will call your command *again*.
//
// special care must be taken to avoid popping values off of the stack!
// when yielded, the vm will call your command again, and if your command
// pops values off of the stack, it will do that *twice*.
typedef bool (*command_fn)(VM *vm, Value *out, u32 arg_count,
                           Resources *resources);

typedef struct
{
    char *name;
    command_fn fn;
} CommandData;

extern const CommandData COMMANDS[Command_Max_Val];
