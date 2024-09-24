#include <stdio.h>
#include <assert.h>
#include "utility/hashset.h"

int main()
{
    HashSet set;
    hashset_init(&set, fnv_hash_function, memcmp_eq_function, sizeof(i32));

    i32 a = 1;
    i32 b = 2;
    i32 c = 3;
    i32 d = 4;

    hashset_insert(&set, &a);
    hashset_insert(&set, &b);
    hashset_insert(&set, &c);
    hashset_insert(&set, &d);

    assert(set.len == 4);

    assert(hashset_contains(&set, &a));
    assert(hashset_contains(&set, &b));
    assert(hashset_contains(&set, &c));
    assert(hashset_contains(&set, &d));

    hashset_remove(&set, &a);
    assert(!hashset_contains(&set, &a));

    HashSetIter iter;
    hashset_iter_init(&set, &iter);

    i32 *key = hashset_iter_next(&iter);
    bool found_b = false;
    bool found_c = false;
    bool found_d = false;
    while (key != NULL)
    {
        switch (*key)
        {
        case 2:
            found_b = true;
            break;
        case 3:
            found_c = true;
            break;
        case 4:
            found_d = true;
            break;
        default:
            assert(false);
        }

        key = hashset_iter_next(&iter);
    }
    assert(found_b);
    assert(found_c);
    assert(found_d);

    hashset_clear(&set);
    assert(!hashset_contains(&set, &b));
    assert(!hashset_contains(&set, &c));
    assert(!hashset_contains(&set, &d));

    // add a bunch of elements
    for (i32 i = 0; i < 32; i++)
    {
        bool is_new = hashset_insert(&set, &i);
        assert(is_new);
    }

    assert(set.len == 32);
    i32 found = 0;
    for (i32 i = 0; i < 32; i++)
    {
        if (hashset_contains(&set, &i))
        {
            found++;
        }
    }
    assert(found == 32);

    hashset_free(&set);
}