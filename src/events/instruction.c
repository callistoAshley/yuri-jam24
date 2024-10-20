#include "instruction.h"
#include <stdio.h>

void print_instruction(Instruction instruction)
{
    switch (instruction.code)
    {
    case Code_Goto:
        printf("goto %d", instruction.data.instruction);
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
        printf("divode");
        break;
    case Code_Mod:
        printf("modulo");
        break;
    case Code_Int:
        printf("int %d", instruction.data._int);
        break;
    case Code_Float:
        printf("float %f", instruction.data._float);
        break;
    case Code_String:
        printf("string '%s'", instruction.data.string);
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
