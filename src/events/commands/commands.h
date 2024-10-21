#pragma once

#include "events/value.h"
#include "command.h"
#include "events/vm.h"
#include "scenes/scene.h"

typedef Value (*command_fn)(VM *vm, u32 arg_count, Resources *resources);

typedef struct
{
    char *name;
    command_fn fn;
} CommandData;

extern const CommandData COMMANDS[Command_Max_Val];
