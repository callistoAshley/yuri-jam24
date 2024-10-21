#include "vm.h"
#include "events/commands/commands.h"
#include "events/value.h"
#include "utility/macros.h"
#include <stdio.h>
#include <string.h>

void vm_init(VM *vm, Event event)
{
    vm->event = event;

    memset(vm->slots, 0, sizeof(vm->slots));

    memset(vm->stack, 0, sizeof(vm->stack));
    vm->top = 0;

    vm->ip = 0;

    memset(vm->command_ctx, 0, sizeof(vm->command_ctx));
}

static inline void push(VM *vm, Value value)
{
    vm->stack[vm->top] = value;
    vm->top++;
}

static inline Value pop(VM *vm)
{
    vm->top--;
    Value value = vm->stack[vm->top];
    return value;
}

static inline Value peek(VM *vm, u32 idx) { return vm->stack[idx]; }

void vm_push(VM *vm, Value value) { push(vm, value); }
Value vm_pop(VM *vm) { return pop(vm); }
Value vm_peek(VM *vm, u32 index) { return peek(vm, index); }

// because we're working with a stack, the right operand comes before the left
// one
// TODO write macro for to clean up the if VAL_IS_INT(v2) code
#define BINARY_CMP_OP(op)                                                      \
    Value v2 = pop(vm);                                                        \
    Value v1 = pop(vm);                                                        \
    if (!VAL_IS_NUMERIC(v1) && !VAL_IS_NUMERIC(v2))                            \
    {                                                                          \
        FATAL("Operands to " #op " must be numbers")                           \
    }                                                                          \
    Value out;                                                                 \
    if (VAL_IS_INT(v1))                                                        \
    {                                                                          \
        if (VAL_IS_INT(v2))                                                    \
            out = BOOL_VAL(v1.data._int op v2.data._int);                      \
        else                                                                   \
            out = BOOL_VAL(v1.data._int op v2.data._float);                    \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        if (VAL_IS_INT(v2))                                                    \
            out = BOOL_VAL(v1.data._float op v2.data._int);                    \
        else                                                                   \
            out = BOOL_VAL(v1.data._float op v2.data._float);                  \
    }                                                                          \
    push(vm, out);

// if *any* of the operands are floats, the output value is a float
#define BINARY_OP(op)                                                          \
    Value v2 = pop(vm);                                                        \
    Value v1 = pop(vm);                                                        \
    if (!VAL_IS_NUMERIC(v1) && !VAL_IS_NUMERIC(v2))                            \
    {                                                                          \
        FATAL("Operands to " #op " must be numbers")                           \
    }                                                                          \
    Value out;                                                                 \
    if (VAL_IS_INT(v1))                                                        \
    {                                                                          \
        if (VAL_IS_INT(v2))                                                    \
            out = INT_VAL(v1.data._int op v2.data._int);                       \
        else                                                                   \
            out = FLOAT_VAL(v1.data._int op v2.data._float);                   \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        if (VAL_IS_INT(v2))                                                    \
            out = FLOAT_VAL(v1.data._float op v2.data._int);                   \
        else                                                                   \
            out = FLOAT_VAL(v1.data._float op v2.data._float);                 \
    }                                                                          \
    push(vm, out);

bool vm_execute(VM *vm, Resources *resources)
{
    while (vm->ip < vm->event.instructions_len)
    {
        Instruction insn = vm->event.instructions[vm->ip];
        vm->ip++;

        switch (insn.code)
        {
        case Code_Goto:
        {
            vm->ip = insn.data.position;
            break;
        }
        case Code_GotoIfFalse:
        {
            Value cond = peek(vm, 0);
            if (value_is_falsey(cond))
                vm->ip = insn.data.position;
            break;
        }
        case Code_GotoIfTrue:
        {
            Value cond = peek(vm, 0);
            if (value_is_truthy(cond))
                vm->ip = insn.data.position;
            break;
        }
        case Code_Call:
        {
            Value value = NONE_VAL;
            command_fn command = COMMANDS[insn.data.call.command].fn;
            bool yield =
                command(vm, &value, insn.data.call.arg_count, resources);
            if (yield)
            {
                // subtract 1 from the instruction pointer so we can call this
                // command again
                vm->ip--;
                return false;
            }
            push(vm, value);
            break;
        }
        case Code_Pop:
        {
            pop(vm);
            break;
        }
        case Code_Fetch:
        {
            Value value = vm->slots[insn.data.slot];
            push(vm, value);
            break;
        }
        case Code_Set:
        {
            Value value = peek(vm, 0);
            vm->slots[insn.data.slot] = value;
            break;
        }
        case Code_Not:
        {
            Value value = pop(vm);
            bool is_falsey = value_is_falsey(value);
            push(vm, BOOL_VAL(is_falsey));
            break;
        }
        case Code_Negate:
        {
            Value value = pop(vm);
            if (!VAL_IS_NUMERIC(value))
            {
                FATAL("Negate operand must be a number");
            }
            if (VAL_IS_INT(value))
                push(vm, INT_VAL(-value.data._int));
            else
                push(vm, FLOAT_VAL(-value.data._float));
            break;
        }
        case Code_Add:
        {
            BINARY_OP(+);
            break;
        }
        case Code_Sub:
        {
            BINARY_OP(-);
            break;
        }
        case Code_Mul:
        {
            BINARY_OP(*);
            break;
        }
        case Code_Div:
        {
            BINARY_OP(/);
            break;
        }
        case Code_Mod:
        {
            Value v2 = pop(vm);
            Value v1 = pop(vm);
            if (!VAL_IS_INT(v1) || !VAL_IS_INT(v2))
            {
                FATAL("Operands to %% must be integers")
            }
            Value out = INT_VAL(v1.data._int % v2.data._int);
            push(vm, out);
            break;
        }
        case Code_Int:
            push(vm, INT_VAL(insn.data._int));
            break;
        case Code_Float:
            push(vm, FLOAT_VAL(insn.data._float));
            break;
        case Code_String:
            push(vm, STRING_VAL(insn.data.string));
            break;
        case Code_True:
            push(vm, TRUE_VAL);
            break;
        case Code_False:
            push(vm, FALSE_VAL);
            break;
        case Code_None:
            push(vm, NONE_VAL);
            break;
        case Code_Eq:
        {
            Value v2 = pop(vm);
            Value v1 = pop(vm);
            bool eq = value_is_eq(v1, v2);
            push(vm, BOOL_VAL(eq));
            break;
        }
        case Code_NotEq:
        {
            Value v2 = pop(vm);
            Value v1 = pop(vm);
            bool eq = value_is_eq(v1, v2);
            push(vm, BOOL_VAL(!eq));
            break;
        }
        case Code_Greater:
        {
            BINARY_CMP_OP(>);
            break;
        }
        case Code_GreaterEq:
        {
            BINARY_CMP_OP(>=);
            break;
        }
        case Code_Less:
        {
            BINARY_CMP_OP(<);
            break;
        }
        case Code_LessEq:
        {
            BINARY_CMP_OP(<=);
            break;
        }
        }
    }

    // we're out of commands, so we've finished.
    return true;
}

void vm_free(VM *vm) { (void)vm; }
