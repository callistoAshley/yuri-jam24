#pragma once

#include "events/event.h"
#include "events/value.h"
#include "scenes/scene.h"

#define STACK_MAX 64
#define SLOT_MAX 64

typedef struct
{
    Event event;

    Value stack[64];
    u32 top;

    Value slots[SLOT_MAX];

    u32 ip;
    // commands are free to modify this pointer so they can maintain state
    // if they yield.
    void *command_ctx;
} VM;

void vm_init(VM *vm, Event event);
// returns true if execution has finished.
bool vm_execute(VM *vm, Resources *resources);
void vm_free(VM *vm);

void vm_push(VM *vm, Value value);
Value vm_pop(VM *vm);
Value vm_peek(VM *vm, u32 index);
