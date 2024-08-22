#include "linked-list.h"
#include <assert.h>

LinkedList *linked_list_init(void)
{
    LinkedList *list;

    list = calloc(1, sizeof(LinkedList));
    if (!list)
        return NULL;

    return list;
}

void linked_list_append(LinkedList *list, void *elem)
{
    LinkedListNode *top;

    if (list->first)
    {
        for (top = list->first; top->next; top = top->next)
        {
        }
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

int linked_list_index_of(LinkedList *list, void *elem)
{
    int i = 0;
    for (LinkedListNode *node = list->first; node; node = node->next)
    {
        if (node->data == elem)
            return i;
        i++;
    }
    return -1;
}

void *linked_list_remove(LinkedList *list, int index)
{
    LinkedListNode *node = list->first;
    for (int i = 0; i < index - 1; i++)
    {
        assert(node);
        node = node->next;
    }

    LinkedListNode *next = node->next;
    void *data = next->data;
    node->next = next->next;
    free(next);
    list->len--;
    return data;
}

void *linked_list_at(LinkedList *list, int index)
{
    LinkedListNode *node = list->first;

    for (int i = 0; i < index; i++)
    {
        if (!node)
            return NULL;
        node = node->next;
    }

    return node->data;
}

void linked_list_free(LinkedList *list)
{
    for (LinkedListNode *node = list->first; node;)
    {
        LinkedListNode *next = node->next;
        free(node);
        node = next;
    }
    free(list);
}
