#pragma once

#include "events/event_loader.h"
#include "sensible_nums.h"

typedef struct
{
    Event *event;

    enum
    {
        State_Executing,
        State_Waiting,
    } state;

    // currently executing instruction
    u32 instruction;
} Interpreter;

void interpreter_init(Interpreter *interpeter, Event *event);
void interpreter_update(Interpreter *interpeter);
