#pragma once
#include "sensible_nums.h"
#include "hashmap.h"
#include <stdbool.h>

// a growable set of keys that contains no duplicates.
typedef HashMap HashSet;

void hashset_init(HashSet *set, hash_function *hash, eq_function *eq,
                  usize key_size);
void hashset_free(HashSet *set);

// returns true if the key was inserted, false if it was already present.
bool hashset_insert(HashSet *set, void *key);
// returns true if the key was removed, false if it was not present.
bool hashset_remove(HashSet *set, void *key);
// returns true if the key is present in the set.
bool hashset_contains(HashSet *set, void *key);

void hashset_clear(HashSet *set);

typedef HashMapIter HashSetIter;

void hashset_iter_init(HashSet *set, HashSetIter *iter);
void *hashset_iter_next(HashSetIter *iter);
