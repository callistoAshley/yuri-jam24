#pragma once

#include "events/event.h"
#include "events/value.h"
#include "resources.h"

#define STACK_MAX 32
#define SLOT_MAX 32
#define COMMAND_CTX_MAX 64

typedef struct
{
    Event event;

    Value stack[STACK_MAX];
    Value slots[SLOT_MAX];

    // command specific context
    char command_ctx[COMMAND_CTX_MAX];
    // vm specific context
    void *vm_ctx;

    u32 top;
    u32 ip;
} VM;

void vm_init(VM *vm, Event event);
// returns true if execution has finished.
bool vm_execute(VM *vm, Resources *resources);
void vm_free(VM *vm);

void vm_push(VM *vm, Value value);
Value vm_pop(VM *vm);
Value vm_peek(VM *vm, u32 index);
