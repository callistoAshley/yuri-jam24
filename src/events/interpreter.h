#pragma once

#include "events/event_loader.h"
#include "scenes/scene.h"
#include "sensible_nums.h"

typedef struct
{
    Event *event;

    enum
    {
        Currently_Executing,
        Currently_Waiting,
    } currently;

    union
    {
        struct
        {
            enum
            {
                Waiting_OnText,
                Waiting_OnTimer
            } on;

            union
            {
                f32 timer;
            } data;
        } waiting;
    } state;

    // currently executing instruction
    u32 instruction;
} Interpreter;

void interpreter_init(Interpreter *interpeter, Event *event);
void interpreter_update(Interpreter *interpeter, Resources *resources);
