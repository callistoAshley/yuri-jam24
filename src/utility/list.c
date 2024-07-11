#include "list.h"

static void realloc_list_data(List *list)
{
    void *temp_data = list->data;

    temp_data = realloc(temp_data, list->cap * sizeof(char *));
    if (!temp_data) 
    {
        list_free(list);
        return;
    }
    list->data = temp_data;
}

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
        list->cap += 8;
        realloc_list_data(list);
    }
}

void list_remove(List *list, char *elem)
{
    for (int i = 0; i < list->len; i++)
    {
        if (list_at(list, i) == elem)
        {
            if (list->len - i > 1) memmove(list->data + i, list->data + i + 1, sizeof(char *) * (list->len - i));
            list->len--;
            if (list->cap - list->len > 8)
            {
                list->cap -= 8;
                realloc_list_data(list); 
            }
            return;
        }
    }
}

void list_remove_at(List *list, int index)
{
    if (list->len - index > 1) memmove(list->data + index, list->data + index + 1, sizeof(char *) * (list->len - index));
    list->len--;
    if (list->cap - list->len > 8)
    {
        list->cap -= 8;
        realloc_list_data(list); 
    }
}

void list_free(List *list)
{
    free(list->data);
    free(list);
}
