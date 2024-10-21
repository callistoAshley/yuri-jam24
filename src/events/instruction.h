#pragma once

#include "sensible_nums.h"

typedef struct
{
    enum InstructionCode
    {
        // go to an instruction
        Code_Goto,
        // go to an instruction if the specified condition is false
        Code_GotoIfFalse,
        // go to an instruction if the specified condition is true
        Code_GotoIfTrue,

        // call an in-built command
        Code_Call,

        // Pop something off of the stack
        Code_Pop,
        // Fetch a value from a slot
        Code_Fetch,
        // Set a value in a slot
        Code_Set,

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

        // comparison
        Code_Eq,
        Code_NotEq,
        Code_Greater,
        Code_GreaterEq,
        Code_Less,
        Code_LessEq,
    } code;
    union
    {
        // goto this position
        u32 position;

        // the slot to fetch/set to/from
        u32 slot;

        // the integer value
        i32 _int;
        // the float value
        f32 _float;
        // the string value
        char *string;
        // the command to call
        struct
        {
            char *command;
            u32 arg_count;
        } call;
    } data;
} Instruction;

typedef enum InstructionCode InstructionCode;

void print_instruction(Instruction instruction);
