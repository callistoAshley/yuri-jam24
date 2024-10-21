#pragma once

#include "events/value.h"

struct VM;

typedef Value (*command_fn)(struct VM *vm, u32 arg_count);

typedef struct
{
    char *name;
    command_fn fn;
} CommandData;

typedef enum
{
    CMD_Printf = 0,
    CMD_Text,
    CMD_Wait,

    Command_Max_Val,
} Command;

extern const CommandData COMMANDS[Command_Max_Val];
