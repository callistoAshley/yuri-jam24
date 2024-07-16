#include <stdio.h>
#include <assert.h>
#include "utility/vec.h"

int main()
{
    vec int_vec;
    vec_init(&int_vec, sizeof(int));

    for (int i = 0; i < 10; i++)
    {
        vec_push(&int_vec, &i);
    }

    for (int i = 0; i < 10; i++)
    {
        int *elem = vec_get(&int_vec, i);
        assert(*elem == i);
    }

    for (int i = 0; i < 10; i++)
    {
        int elem;
        vec_pop(&int_vec, &elem);
        assert(elem == 9 - i);
    }

    for (int i = 0; i < 10; i++)
    {
        vec_push(&int_vec, &i);
    }
    int insert_elem = 42;
    vec_insert(&int_vec, 5, &insert_elem);

    int *indexed_elem = vec_get(&int_vec, 5);
    assert(*indexed_elem == 42);
    indexed_elem = vec_get(&int_vec, 6);
    assert(*indexed_elem == 5);
    indexed_elem = vec_get(&int_vec, 4);
    assert(*indexed_elem == 4);

    vec_remove(&int_vec, 5, NULL);
    indexed_elem = vec_get(&int_vec, 5);
    assert(*indexed_elem == 5);

    vec_free(&int_vec);
}