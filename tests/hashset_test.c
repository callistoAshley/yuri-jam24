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

    assert(hashset_contains(&set, &a));
    assert(hashset_contains(&set, &b));
    assert(hashset_contains(&set, &c));
    assert(hashset_contains(&set, &d));

    hashset_remove(&set, &a);
    assert(!hashset_contains(&set, &a));

    hashset_clear(&set);
    assert(!hashset_contains(&set, &b));
    assert(!hashset_contains(&set, &c));
    assert(!hashset_contains(&set, &d));

    hashset_free(&set);
}