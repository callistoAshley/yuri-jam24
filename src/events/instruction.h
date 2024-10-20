#pragma once

#include "sensible_nums.h"

typedef struct
{
    enum InstructionCode
    {
        // go to an instruction
        Code_Goto,

        // unary ops
        Code_Negate,
        Code_Not,

        // math ops
        Code_Add,
        Code_Sub,
        Code_Mul,
        Code_Div,
        Code_Mod,

        // literals
        Code_Int,
        Code_Float,
        Code_String,
        Code_True,
        Code_False,
        Code_None,
    } code;
    union
    {
        // goto this instruction
        u32 instruction;

        // the integer value
        i32 _int;
        // the float value
        f32 _float;
        // the string value
        char *string;
    } data;
} Instruction;

typedef enum InstructionCode InstructionCode;

void print_instruction(Instruction instruction);
