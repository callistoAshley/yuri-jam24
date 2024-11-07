#include "commands.h"
#include "characters/basic.h"
#include "events/commands/command.h"
#include "events/value.h"
#include "events/vm.h"
#include "scenes/map.h"
#include "utility/macros.h"
#include "resources.h"

#define ARG_ERROR(name, expected)                                              \
    if (arg_count != expected)                                                 \
    {                                                                          \
        FATAL("wrong arity (%d) for " #name "\n", arg_count)                   \
    }

#define CLEAR_CTX(vm) memset(vm->command_ctx, 0, sizeof(vm->command_ctx));

// FIXME: argument type validation

static bool cmd_printf(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)resources;
    (void)out;
    ARG_ERROR("printf", 1);

    Value val = vm_pop(vm);
    switch (val.type)
    {
    case Val_None:
        printf("none\n");
        break;
    case Val_Int:
        printf("%i\n", val.data._int);
        break;
    case Val_Float:
        printf("%f\n", val.data._float);
        break;
    case Val_String:
        printf("%s\n", val.data.string);
        break;
    case Val_True:
        printf("true\n");
        break;
    case Val_False:
        printf("false\n");
        break;
    }
    return false;
}

static bool cmd_wait(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    struct WaitCtx
    {
        f32 waited;
    };

    (void)out;
    (void)resources;
    ARG_ERROR("wait", 1);

    Value wait_val = vm_peek(vm, vm->top - 1);
    f32 wait_time = wait_val.data._float;

    struct WaitCtx *ctx = (struct WaitCtx *)vm->command_ctx;

    // we're done waiting, stop yielding
    if (ctx->waited >= wait_time)
    {
        CLEAR_CTX(vm);
        vm_pop(vm);
        return false;
    }

    ctx->waited += time_delta_seconds(resources->time.current);
    return true;
}

static bool cmd_text(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    struct TextCtx
    {
        bool has_started;
    };

    (void)out;
    ARG_ERROR("text", 1);

    struct TextCtx *ctx = (struct TextCtx *)vm->command_ctx;
    MapScene *scene = (MapScene *)resources->scene;

    if (ctx->has_started)
    {
        // we've already displayed the text, and are waiting for the textbox to
        // finish.
        if (scene->textbox.open)
            return true;

        // we're done waiting, stop yielding, and pop the argument off the stack
        CLEAR_CTX(vm);
        vm_pop(vm);
        return false;
    }

    // indicate that we are waiting for the textbox to finish
    Value text_val = vm_peek(vm, vm->top - 1);
    char *text = text_val.data.string;
    textbox_display_text(&scene->textbox, resources, text);
    ctx->has_started = true;

    return true;
}

static bool cmd_yield(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    struct YieldCtx
    {
        bool did_yield;
    };

    (void)vm;
    (void)arg_count;
    (void)resources;
    (void)out;
    ARG_ERROR("yield", 0);

    struct YieldCtx *ctx = (struct YieldCtx *)vm->command_ctx;

    if (ctx->did_yield)
    {
        // we just yielded, so clear the ctx and return immediately
        CLEAR_CTX(vm);
        return false;
    }

    ctx->did_yield = true;
    return true;
}

static bool cmd_rand(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)vm;
    (void)arg_count;
    (void)resources;

    ARG_ERROR("rand", 0);

    *out = INT_VAL(rand());

    return false;
}

static bool cmd_move_l(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)out;

    ARG_ERROR("move_l", 0);

    BasicCharState *state = vm->vm_ctx;
    state->transform.position.x -= 8.0;
    transform_manager_update(&resources->graphics.transform_manager,
                             state->sprite.transform, state->transform);

    return false;
}

static bool cmd_move_r(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)out;

    ARG_ERROR("move_r", 0);

    BasicCharState *state = vm->vm_ctx;
    state->transform.position.x += 8.0;
    transform_manager_update(&resources->graphics.transform_manager,
                             state->sprite.transform, state->transform);

    return false;
}

static bool cmd_move(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)out;

    ARG_ERROR("move", 1);

    BasicCharState *state = vm->vm_ctx;
    Value amount = vm_pop(vm);
    switch (amount.type)
    {
    case Val_Int:
        state->transform.position.x += amount.data._int;
        break;
    case Val_Float:
        state->transform.position.x += amount.data._float;
        break;
    default:
        FATAL("Wrong type to cmd_move (expected number, got %d)\n", amount.type)
    }
    transform_manager_update(&resources->graphics.transform_manager,
                             state->sprite.transform, state->transform);

    return false;
}

static bool cmd_change_map(VM *vm, Value *out, u32 arg_count,
                           Resources *resources)
{
    (void)out;

    MapScene *scene = (MapScene *)resources->scene;
    scene->change_map_args.copy_map_path = true;
    scene->change_map = true;
    if (arg_count == 3)
    {
        scene->change_map_args.initial_position.y = vm_pop(vm).data._float;
        scene->change_map_args.initial_position.x = vm_pop(vm).data._float;
    }
    else if (arg_count != 1)
    {
        FATAL("wrong arity (%d) for change_map \n", arg_count)
    }
    scene->change_map_args.map_path = vm_pop(vm).data.string;

    // set the instruction pointer to the
    // end of the event to exit execution
    vm->ip = vm->event.instructions_len;

    return false;
}

static bool cmd_exit(VM *vm, Value *out, u32 arg_count, Resources *resources)
{
    (void)out;
    (void)resources;

    ARG_ERROR("exit", 0);

    // set the instruction pointer to the
    // end of the event to exit execution
    vm->ip = vm->event.instructions_len;

    return false;
}

static bool unimplemented(VM *vm, Value *out, u32 arg_count,
                          Resources *resources)
{
    (void)vm;
    (void)arg_count;
    (void)resources;
    (void)out;
    FATAL("Unimplemented command!\n");
}

const CommandData COMMANDS[] = {
    [CMD_Printf] = {"printf", cmd_printf},
    [CMD_Text] = {"text", cmd_text},
    [CMD_Wait] = {"wait", cmd_wait},
    [CMD_Yield] = {"yield", cmd_yield},
    [CMD_Rand] = {"rand", cmd_rand},

    [CMD_MoveL] = {"move_l", cmd_move_l},
    [CMD_MoveR] = {"move_r", cmd_move_r},
    [CMD_Move] = {"move", cmd_move},

    [CMD_ChangeMap] = {"change_map", cmd_change_map},
    [CMD_Exit] = {"exit", cmd_exit},

    [CMD_Unimplemented] = {"unimplemented", unimplemented},
};
