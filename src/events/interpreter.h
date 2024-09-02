#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "lexer.h"
#include "token.h"
#include "../utility/linked_list.h"
#include "../utility/macros.h"

typedef struct
{
    char *name;
    int num_tokens;
    Token *tokens;
} Event;

typedef struct
{
    LinkedList *events;

    int cond_register;
    char choice_buffer[256];
} Interpreter;

Interpreter *interpreter_init(char **files, int num_files);
void interpreter_free(Interpreter *interpreter);
