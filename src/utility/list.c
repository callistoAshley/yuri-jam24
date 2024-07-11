#include "list.h"

List *list_init(void)
{
    List *list = calloc(1, sizeof(List));

    if (!list) 
    {
        return NULL;
    }
    list->cap = 8;
    list->data = calloc(list->cap, sizeof(char *));
    if (!list->data)
    {
        free(list);
        return NULL;
    }

    return list;
}

char *list_at(List *list, int index)
{
    return list->data[index];
}

void list_append(List *list, char *elem)
{
    list->data[list->len++] = elem;

    if (list->len >= list->cap) 
    {
        void *temp_data = list->data;

        list->cap += 8;
        temp_data = realloc(temp_data, list->cap * sizeof(char *));
        if (!temp_data) 
        {
            list_free(list);
            return;
        }
        list->data = temp_data;
    }
}

void list_free(List *list)
{
    free(list->data);
    free(list);
}
