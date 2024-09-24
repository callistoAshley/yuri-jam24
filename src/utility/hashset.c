#include "hashset.h"
#include "sensible_nums.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hashset_init(HashSet *set, hash_function *hash, eq_function *eq,
                  usize key_size)
{
    hashmap_init(set, hash, eq, key_size, 0);
}

void hashset_free(HashSet *set) { hashmap_free(set); }

bool hashset_insert(HashSet *set, void *key)
{
    return hashmap_insert(set, key, NULL);
}

bool hashset_remove(HashSet *set, void *key)
{
    return hashmap_remove(set, key, NULL);
}

bool hashset_contains(HashSet *set, void *key)
{
    return hashmap_contains(set, key);
}

void hashset_clear(HashSet *set) { hashmap_clear(set); }

void hashset_iter_init(HashSet *set, HashSetIter *iter)
{
    hashmap_iter_init(set, iter);
}

void *hashset_iter_next(HashSetIter *iter)
{
    void *key;
    bool not_done = hashmap_iter_next(iter, &key, NULL);
    return not_done ? key : NULL;
}