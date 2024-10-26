#include "commands.h"
#include "events/commands/command.h"
#include "events/vm.h"
#include "scenes/map.h"
#include "scenes/scene.h"
#include "utility/macros.h"

#define ARG_ERROR(name, expected)                                              \
    if (arg_count != expected)                                                 \
    {                                                                          \
        FATAL("wrong arity (%d) for " #name, arg_count)                        \
    }

#define CLEAR_CTX(vm) memset(vm->command_ctx, 0, sizeof(vm->command_ctx));

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
        u32 count;
    };

    (void)out;
    (void)resources;
    ARG_ERROR("wait", 1);

    Value wait_val = vm_peek(vm, vm->top - 1);
    u32 wait_count = wait_val.data._int;

    struct WaitCtx *ctx = (struct WaitCtx *)vm->command_ctx;

    // we're done waiting, stop yielding
    if (ctx->count >= wait_count)
    {
        CLEAR_CTX(vm);
        vm_pop(vm);
        return false;
    }

    printf("waited %d times\n", ctx->count);
    ctx->count++;
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
    MapScene *scene = (MapScene *)*resources->current_scene;

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
    [CMD_Unimplemented] = {"unimplemented", unimplemented},
};
