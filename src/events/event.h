#pragma once
#include <stdbool.h>

#include "../utility/linked-list.h"

struct CommandFn;

typedef int (*CommandFn)(struct CommandFn *lua);

typedef struct CommandFn
{
    CommandFn func;
    LinkedList *args;
    bool is_userdatum; // whether this command is a lua userdatum
} Command;

typedef struct
{
    LinkedList *commands;    
    int command_index;
} Event;

Event *event_init(LinkedList *commands);
void event_free(Event *event);
