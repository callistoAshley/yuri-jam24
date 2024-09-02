#include <stdio.h>
#include <assert.h>
#include "utility/linked_list.h"

int main(void)
{
    LinkedList *list = linked_list_init();

    int one = 1, two = 2, three = 3, four = 4, five = 5;

    linked_list_append(list, &one);
    assert(linked_list_at(list, 0) == &one);

    linked_list_append(list, &two);
    assert(linked_list_at(list, 1) == &two);

    linked_list_append(list, &three);
    assert(linked_list_at(list, 2) == &three);

    linked_list_append(list, &four);
    assert(linked_list_at(list, 3) == &four);

    linked_list_append(list, &five);
    assert(linked_list_at(list, 4) == &five);

    assert(linked_list_remove(list, 2) == &three);
    assert(linked_list_at(list, 2) == &four);

    assert(linked_list_remove(list, 0) == &one);
    assert(linked_list_at(list, 0) == &two);

    assert(linked_list_remove(list, list->len - 1) == &five);
    assert(linked_list_at(list, list->len - 1) == &four);

    linked_list_free(list);
}
