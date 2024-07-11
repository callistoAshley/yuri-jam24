#pragma once
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    size_t len, cap;
    char **data;
} List;

List *list_init(void);
char *list_at(List *list, int index);
void list_append(List *list, char *elem);
void list_free(List *list);
