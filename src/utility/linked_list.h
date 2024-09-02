#pragma once
#include <stdlib.h>
#include <stdio.h>

typedef struct LinkedListNode
{
    struct LinkedListNode *next;
    void *data;
} LinkedListNode;

typedef struct
{
    LinkedListNode *first;
    int len;
} LinkedList;

LinkedList *linked_list_init(void);
void linked_list_append(LinkedList *list, void *elem);
int linked_list_index_of(LinkedList *list, void *elem);
void *linked_list_remove(LinkedList *list, int index);
void *linked_list_at(LinkedList *list, int index);
void linked_list_free(LinkedList *list);
