#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "lexer.h"
#include "sensible_nums.h"
#include "token.h"
#include "../utility/linked_list.h"
#include "../utility/macros.h"

typedef struct
{
    char *name;
    u32 num_tokens;
    Instruction *instructions;
} Event;

typedef struct
{
    LinkedList *events;

} EventLoader;

EventLoader *event_loader_init(char **files, int num_files);
void event_loader_free(EventLoader *interpreter);
