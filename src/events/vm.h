#pragma once

#include "events/event.h"
#include "events/value.h"

#define STACK_MAX 64
#define SLOT_MAX 64

typedef struct
{
    Event event;

    Value stack[64];
    u32 top;

    Value slots[SLOT_MAX];

    u32 ip;
} VM;

void vm_init(VM *vm, Event event);
void vm_execute(VM *vm);
void vm_free(VM *vm);
