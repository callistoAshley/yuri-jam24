#include <stdio.h>
#include <assert.h>
#include "utility/hashmap.h"
#include "utility/hashset.h"

int main()
{
    HashMap map;
    hashmap_init(&map, fnv_hash_function, memcmp_eq_function, sizeof(i32),
                 sizeof(i32));

    i32 a = 1;
    i32 b = 2;
    i32 c = 3;
    i32 d = 4;

    hashmap_insert(&map, &a, &d);
    hashmap_insert(&map, &b, &c);
    hashmap_insert(&map, &c, &b);
    hashmap_insert(&map, &d, &a);

    assert(map.len == 4);

    assert(hashmap_contains(&map, &a));
    assert(hashmap_contains(&map, &b));
    assert(hashmap_contains(&map, &c));
    assert(hashmap_contains(&map, &d));

    i32 *value_p = hashmap_get(&map, &a);
    assert(*value_p == d);
    value_p = hashmap_get(&map, &b);
    assert(*value_p == c);
    value_p = hashmap_get(&map, &c);
    assert(*value_p == b);
    value_p = hashmap_get(&map, &d);
    assert(*value_p == a);

    i32 value;
    hashmap_remove(&map, &a, &value);
    assert(!hashmap_contains(&map, &a));
    assert(value == d);

    hashmap_remove(&map, &b, NULL);
    assert(!hashmap_contains(&map, &b));

    HashMapIter iter;
    hashmap_iter_init(&map, &iter);

    i32 *key;
    bool found_c = false;
    bool found_d = false;

    while (hashmap_iter_next(&iter, (void **)&key, (void **)&value_p))
    {
        switch (*key)
        {
        case 3:
            assert(*value_p == b);
            found_c = true;
            break;
        case 4:
            assert(*value_p == a);
            found_d = true;
            break;
        default:
            assert(false);
        }
    }

    assert(found_c);
    assert(found_d);

    hashmap_free(&map);
}