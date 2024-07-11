#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    size_t len, cap;
    char **data;
} List;

List *list_init(void);
char *list_at(List *list, int index);
void list_append(List *list, char *elem);
void list_remove(List *list, char *elem);
void list_remove_at(List *list, int index);
void list_free(List *list);
