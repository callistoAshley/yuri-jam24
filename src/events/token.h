#pragma once

typedef enum
{
    TOKEN_TEXT,
    TOKEN_CMD
} TokenType;

typedef struct
{
    TokenType type;
    union
    {
        char text[256];
        struct
        {
            char name[256];
            char arg[256];
        } command;
    };
} Token;
