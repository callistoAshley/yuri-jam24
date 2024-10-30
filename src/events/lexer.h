#pragma once

#include "sensible_nums.h"
#include <stdbool.h>
typedef struct
{
    enum TokenType
    {
        Token_Semicolon,
        // keywords
        Token_Event,
        Token_Goto,
        Token_If,
        Token_Else,
        Token_Loop,
        Token_While,
        Token_For,
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

        // assignment ops
        Token_Set,
        Token_SetAdd,
        Token_SetSub,
        Token_SetMult,
        Token_SetDiv,
        Token_SetMod,
        // increment/decrement ops
        Token_Inc,
        Token_Dec,

        Token_Eq,
        Token_NotEq,

        Token_Less,
        Token_LessEq,

        Token_Greater,
        Token_GreaterEq,

        Token_Not,
        Token_And,
        Token_Or,

        Token_Plus,
        Token_Minus,
        Token_Mult,
        Token_Div,
        Token_Mod,
    } type;

    union TokenData
    {
        char *string;
        char *ident;
        char *label;
        i32 _int;
        f32 _float;
    } data;
} Token;

typedef enum TokenType TokenType;
typedef union TokenData TokenData;
typedef struct
{
    const char *start;
    const char *current;
} Lexer;

void lexer_init(Lexer *lexer, const char *src);
// returns true if there are any more tokens
bool lexer_next(Lexer *lexer, Token *token);
bool lexer_eof(Lexer *lexer);

void token_debug_printf(Token token);
