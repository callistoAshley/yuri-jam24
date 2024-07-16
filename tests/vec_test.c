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

    vec_free(&int_vec);
}