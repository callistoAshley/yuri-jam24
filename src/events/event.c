#include "event.h"
#include "events/commands/commands.h"
#include <stdio.h>
#include <stdlib.h>

static void simple_instruction(const char *name) { printf("%s\n", name); }

static void slot_instruction(const char *name, u32 slot)
{
    printf("%s %d\n", name, slot);
}

static void goto_instruction(const char *name, u32 from, u32 to)
{
    printf("%s %d -> %d\n", name, from, to);
}

static void call_instruction(const char *name, Command command, u32 args)
{
    CommandData data = COMMANDS[command];
    printf("%s %s (%d args)\n", name, data.name, args);
}

static void disassemble_instruction(Event *event, u32 pos)
{
    printf("%04d | ", pos);
    Instruction insn = event->instructions[pos];
    switch (insn.code)
    {
    case Code_Goto:
        goto_instruction("Code_Goto", pos, insn.data.position);
        break;
    case Code_GotoIfFalse:
        goto_instruction("Code_IfFalse", pos, insn.data.position);
        break;
    case Code_GotoIfTrue:
        goto_instruction("Code_GotoIfTrue", pos, insn.data.position);
        break;
    case Code_Call:
        call_instruction("Code_Call", insn.data.call.command,
                         insn.data.call.arg_count);
        break;
    case Code_Pop:
        simple_instruction("Code_Pop");
        break;
    case Code_Fetch:
        slot_instruction("Code_Fetch", insn.data.slot);
        break;
    case Code_Set:
        slot_instruction("Code_Set", insn.data.slot);
        break;
    case Code_Negate:
        simple_instruction("Code_Negate");
        break;
    case Code_Not:
        simple_instruction("Code_Not");
        break;
    case Code_Add:
        simple_instruction("Code_Add");
        break;
    case Code_Sub:
        simple_instruction("Code_Sub");
        break;
    case Code_Mul:
        simple_instruction("Code_Mul");
        break;
    case Code_Div:
        simple_instruction("Code_Div");
        break;
    case Code_Mod:
        simple_instruction("Code_Mod");
        break;
    case Code_Int:
        printf("Code_Int %d\n", insn.data._int);
        break;
    case Code_Float:
        printf("Code_Float %f\n", insn.data._float);
        break;
    case Code_String:
        printf("Code_String %s\n", insn.data.string);
        break;
    case Code_True:
        simple_instruction("Code_True");
        break;
    case Code_False:
        simple_instruction("Code_False");
        break;
    case Code_None:
        simple_instruction("Code_None");
        break;
    case Code_Eq:
        simple_instruction("Code_Eq");
        break;
    case Code_NotEq:
        simple_instruction("Code_NotEq");
        break;
    case Code_Greater:
        simple_instruction("Code_Greater");
        break;
    case Code_GreaterEq:
        simple_instruction("Code_GreaterEq");
        break;
    case Code_Less:
        simple_instruction("Code_Less");
        break;
    case Code_LessEq:
        simple_instruction("Code_LessEq");
        break;
    }
}

void event_disassemble(Event *event)
{
    printf("== %s ==\n", event->name);
    for (u32 pos = 0; pos < event->instructions_len; pos++)
    {
        disassemble_instruction(event, pos);
    }
}

void event_free(Event *event)
{
    free(event->name);

    for (u32 i = 0; i < event->slot_count; i++)
    {
        free(event->slots[i]);
    }
    free(event->slots);

    for (u32 i = 0; i < event->instructions_len; i++)
    {
        Instruction instruction = event->instructions[i];
        switch (instruction.code)
        {
        case Code_String:
            free(instruction.data.string);
            break;
        default:
            break;
        }
    }
    free(event->instructions);
}
