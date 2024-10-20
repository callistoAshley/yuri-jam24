#pragma once

#include "sensible_nums.h"
#include <stdbool.h>
typedef struct
{
    enum TokenType
    {
        // keywords
        Token_Event,
        Token_Goto,
        Token_If,
        Token_Loop,
        Token_Op,
        // keyword values
        Token_None,
        Token_True,
        Token_False,

        // special
        Token_Ident,
        Token_Label,

        // braces (ifs, event defs, etc)
        Token_BraceL,
        Token_BraceR,
        // command calls
        Token_ParenL,
        Token_ParenR,
        Token_Comma,

        // literals
        Token_String,
        Token_Int,
        Token_Float,
    } type;

    union TokenData
    {
        char *string;
        char *ident;
        char *label;
        i32 _int;
        f32 _float;

        // operators <, <=, etc..
        enum OpType
        {
            Op_Set,

            Op_Equals,
            Op_NotEq,

            Op_Less,
            Op_LessEq,

            Op_Greater,
            Op_GreaterEq,

            Op_Not,
            Op_And,
            Op_Or,

            Op_Plus,
            Op_Minus,
            Op_Mult,
            Op_Div,
            Op_Mod,
        } op;
    } data;
} Token;

typedef enum TokenType TokenType;
typedef union TokenData TokenData;
typedef enum OpType OpType;
typedef struct
{
    const char *start;
    const char *current;
} Lexer;

void lexer_init(Lexer *lexer, const char *src);
// returns true if there are any more tokens
bool lexer_next(Lexer *lexer, Token *token);
