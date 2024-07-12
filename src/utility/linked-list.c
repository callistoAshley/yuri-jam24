#include "linked-list.h"

LinkedList *linked_list_init(void)
{
    LinkedList *list;

    list = calloc(1, sizeof(LinkedList));
    if (!list) return NULL;

    return list;
}

void linked_list_append(LinkedList *list, void *elem)
{
    LinkedListNode *top;

    if (list->first)
    {
        for (top = list->first; top->next; top = top->next) {}
        top->next = calloc(1, sizeof(LinkedListNode));
        top = top->next;
    }
    else
    { 
        list->first = calloc(1, sizeof(LinkedListNode));
        top = list->first;
    }

    top->data = elem;
    list->len++;
}

void *linked_list_at(LinkedList *list, int index)
{
    LinkedListNode *node = list->first;

    for (int i = 0; i < index; i++)
    {
        if (!node) return NULL;
        node = node->next;
    }

    return node->data;
}

void linked_list_free(LinkedList *list)
{
    for (LinkedListNode *node = list->first; node; )
    {
        LinkedListNode *next = node->next;
        free(node);
        node = next;
    }
    free(list);
}
