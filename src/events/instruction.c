#include "instruction.h"
#include <stdio.h>

void print_instruction(Instruction insn)
{
    switch (insn.code)
    {
    case Code_Goto:
        printf("goto %d", insn.data.position);
        break;
    case Code_GotoIfFalse:
        printf("goto (if false) %d", insn.data.position);
        break;
    case Code_GotoIfTrue:
        printf("goto (if true) %d", insn.data.position);
        break;
    case Code_Call:
        printf("call %s with %d args", insn.data.call.command,
               insn.data.call.arg_count);
        break;
    case Code_Pop:
        printf("pop");
        break;
    case Code_Fetch:
        printf("fetch %d", insn.data.slot);
        break;
    case Code_Set:
        printf("set %d", insn.data.slot);
        break;
    case Code_Negate:
        printf("negate");
        break;
    case Code_Not:
        printf("not");
        break;
    case Code_Add:
        printf("add");
        break;
    case Code_Sub:
        printf("subtract");
        break;
    case Code_Mul:
        printf("multiply");
        break;
    case Code_Div:
        printf("divide");
        break;
    case Code_Mod:
        printf("modulo");
        break;
    case Code_Int:
        printf("int %d", insn.data._int);
        break;
    case Code_Float:
        printf("float %f", insn.data._float);
        break;
    case Code_String:
        printf("string '%s'", insn.data.string);
        break;
    case Code_True:
        printf("true");
        break;
    case Code_False:
        printf("false");
        break;
    case Code_None:
        printf("none (literal)");
        break;
    case Code_Eq:
        printf("==");
        break;
    case Code_NotEq:
        printf("!=");
        break;
    case Code_Greater:
        printf(">");
        break;
    case Code_GreaterEq:
        printf(">=");
        break;
    case Code_Less:
        printf("<");
        break;
    case Code_LessEq:
        printf("<=");
        break;
    }
}
