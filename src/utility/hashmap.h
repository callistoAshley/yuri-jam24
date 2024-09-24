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
    char data[];
} Bucket;

// a growable map of keys that contains no duplicates.
typedef struct
{
    u32 len;
    u32 capacity;
    usize key_size, value_size;

    Bucket *buckets;

    hash_function *hash;
    eq_function *eq;
} HashMap;

u64 fnv_hash_function(void *key, usize key_size);
bool memcmp_eq_function(void *a, void *b, usize key_size);

// specific hash and eq functions for c strings.
u64 fnv_cstr_hash_function(void *key, usize key_size);
bool memcmp_cstr_eq_function(void *a, void *b, usize key_size);

// NOTE: value_size MAY be 0, but key_size MUST be > 0.
void hashmap_init(HashMap *map, hash_function *hash, eq_function *eq,
                  usize key_size, usize value_size);
void hashmap_free(HashMap *map);

// returns true if the key was inserted, false if it was already present.
// if value_size is 0, value may be NULL.
bool hashmap_insert(HashMap *map, void *key, void *value);
// returns the value associated with the key, or NULL if the key is not present.
// also returns NULL if value_size is 0.
void *hashmap_get(HashMap *map, void *key);
// returns true if the key was removed, false if it was not present.
// if value is not NULL, the value associated with the key is copied into it.
bool hashmap_remove(HashMap *map, void *key, void *value);
// returns true if the key is present in the map.
bool hashmap_contains(HashMap *map, void *key);

// TODO support freeing members of the map.
void hashmap_clear(HashMap *map);

typedef struct
{
    HashMap *map;
    usize index;
} HashMapIter;

void hashmap_iter_init(HashMap *map, HashMapIter *iter);
bool hashmap_iter_next(HashMapIter *iter, void **key, void **value);
