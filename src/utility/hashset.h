#pragma once
#include "sensible_nums.h"
#include <stdbool.h>

typedef u64(hash_function)(void *key, usize key_size);
typedef bool(eq_function)(void *a, void *b, usize key_size);

// NOTE: keys are stored inline with the buckets.
typedef struct
{
    u64 hash;
    bool dead, occupied;
    // this is a flexible array member, so it's not counted in the size of the
    // struct.
    char key[];
} Bucket;

// a growable set of keys that contains no duplicates.
typedef struct
{
    u32 len;
    u32 capacity;
    usize key_size;

    Bucket *buckets;

    hash_function *hash;
    eq_function *eq;
} HashSet;

u64 fnv_hash_function(void *key, usize key_size);
bool memcmp_eq_function(void *a, void *b, usize key_size);

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

typedef struct
{
    HashSet *set;
    usize index;
} HashSetIter;

void hashset_iter_init(HashSet *set, HashSetIter *iter);
void *hashset_iter_next(HashSetIter *iter);
