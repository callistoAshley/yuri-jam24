#include "hashmap.h"
#include "sensible_nums.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// most of this is copied from craftinginterpreters.com (cool book)

u64 fnv_hash_function(void *key, usize key_size)
{
    u64 hash = 0xcbf29ce484222325u;
    u8 *key_bytes = (u8 *)key;
    for (usize i = 0; i < key_size; i++)
    {
        hash ^= key_bytes[i];
        hash *= 0x00000100000001B3;
    }
    return hash;
}

bool memcmp_eq_function(void *a, void *b, usize key_size)
{
    return memcmp(a, b, key_size) == 0;
}

u64 fnv_cstr_hash_function(void *key, usize key_size)
{
    (void)key_size;
    return fnv_hash_function(key, strlen((char *)key));
}

bool memcmp_cstr_eq_function(void *a, void *b, usize key_size)
{
    (void)key_size;
    return strcmp(a, b) == 0;
}

void hashmap_init(HashMap *map, hash_function *hash, eq_function *eq,
                  usize key_size, usize value_size)
{
    map->len = 0;
    map->key_size = key_size;
    map->value_size = value_size;

    // 8 is ok as a starting capacity (but we can always change this behavior
    // later)
    map->capacity = 8;

    usize bucket_size = sizeof(Bucket) + key_size + value_size;
    // keys are stored inline with the buckets
    map->buckets = calloc(map->capacity, bucket_size);

    map->hash = hash;
    map->eq = eq;
}

void hashmap_free(HashMap *map) { free(map->buckets); }

#define TABLE_MAX_LOAD 0.75
// keys are stored inline with the buckets, so we need to do some pointer
// shenanigans
// only use this to read/write to a key!!! this pointer will always be non-null.
#define KEY_OF(b) ((void *)&(b)->data)
#define VALUE_OF(b, __set) ((void *)&(b)->data[__set->key_size])

static Bucket *find_bucket(HashMap *map, void *key, u64 hash)
{
    usize bucket_size = sizeof(Bucket) + map->key_size + map->value_size;
    usize index = hash % map->capacity;
    Bucket *tombstone = NULL;
    for (;;)
    {
        Bucket *bucket = map->buckets + (index * bucket_size);

        if (!bucket->occupied)
        {
            if (tombstone)
                return tombstone;
            else
                return bucket;
        }
        else if (bucket->dead)
        {
            if (!tombstone)
                tombstone = bucket;
        }
        else if (map->eq(KEY_OF(bucket), key, map->key_size))
        {
            return bucket;
        }

        index = (index + 1) % map->capacity;
    }
}

static void adjust_set_capacity(HashMap *map, usize new_capacity)
{
    usize bucket_size = sizeof(Bucket) + map->key_size + map->value_size;
    // calloc zeroes out the memory, which is perfect
    Bucket *new_buckets = calloc(new_capacity, bucket_size);
    Bucket *old_buckets = map->buckets;
    map->buckets = new_buckets;

    // we effectively have to rebuild the map from scratch
    map->len = 0;
    for (usize i = 0; i < map->capacity; i++)
    {
        Bucket *bucket = old_buckets + (i * bucket_size);
        if (!bucket->occupied)
            continue;

        Bucket *new_bucket = find_bucket(map, KEY_OF(bucket), bucket->hash);
        memcpy(new_bucket, bucket, bucket_size);
        new_bucket->occupied = true;
        new_bucket->dead = false;
        map->len++;
    }

    free(old_buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

bool hashmap_insert(HashMap *map, void *key, void *value)
{
    if (map->len + 1 > map->capacity * TABLE_MAX_LOAD)
    {
        usize new_capacity = map->capacity * 2;
        adjust_set_capacity(map, new_capacity);
    }

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, key, hash);
    bool is_new = !bucket->occupied;

    if (is_new)
    {
        // only increment the length if the bucket was not dead.
        // this is because we're not actually filling out a brand new bucket,
        // just reusing a dead one.
        if (!bucket->dead)
            map->len++;
        bucket->hash = hash;
        bucket->occupied = true;
        bucket->dead = false;
        memcpy(KEY_OF(bucket), key, map->key_size);
        if (map->value_size > 0)
            memcpy(VALUE_OF(bucket, map), value, map->value_size);
    }

    return is_new;
}

void *hashmap_get(HashMap *map, void *key)
{
    if (map->len == 0)
        return NULL;
    if (map->value_size == 0)
        return NULL;

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, key, hash);
    if (!bucket->occupied)
        return NULL;

    return VALUE_OF(bucket, map);
}

bool hashmap_remove(HashMap *map, void *key, void *value)
{
    if (map->len == 0)
        return false;

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, key, hash);
    if (!bucket->occupied)
        return false;

    if (value)
        memcpy(value, VALUE_OF(bucket, map), map->value_size);

    // mark the bucket as dead
    bucket->dead = true;
    bucket->occupied = false;
    map->len--;

    return true;
}

bool hashmap_contains(HashMap *map, void *key)
{
    if (map->len == 0)
        return false;
    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, key, hash);
    return bucket->occupied;
}

void hashmap_clear(HashMap *map)
{
    map->len = 0;
    usize bucket_size = sizeof(Bucket) + map->key_size + map->value_size;
    for (usize i = 0; i < map->capacity; i++)
    {
        Bucket *bucket = map->buckets + (i * bucket_size);
        bucket->occupied = false;
        bucket->dead = false;
    }
}

void hashmap_iter_init(HashMap *map, HashMapIter *iter)
{
    iter->map = map;
    iter->index = 0;
}

bool hashmap_iter_next(HashMapIter *iter, void **key, void **value)
{

    HashMap *map = iter->map;
    usize bucket_size = sizeof(Bucket) + map->key_size + map->value_size;
    Bucket *buckets = map->buckets;

    while (iter->index < map->capacity)
    {
        Bucket *bucket = buckets + (iter->index * bucket_size);
        iter->index++;
        if (bucket->occupied && !bucket->dead)
        {
            if (key)
                *key = KEY_OF(bucket);
            if (value)
                *value = VALUE_OF(bucket, map);
            return true;
        }
    }

    return false;
}
