#pragma once

#include "events/instruction.h"

typedef struct
{
    char *name;

    Instruction *instructions;
    u32 instructions_len;

    // used for debug information
    char **slots;
    u32 slot_count;
} Event;

void event_disassemble(Event *event);
void event_free(Event *event);
