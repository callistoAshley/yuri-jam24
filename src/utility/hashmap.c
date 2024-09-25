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

bool strlen_eq_function(void *a, void *b, usize key_size)
{
    (void)key_size;
    return strcmp(a, b) == 0;
}

#define BUCKET_SIZE(map) (sizeof(Bucket) + (map)->key_size + (map)->value_size)
#define BUCKET_AT(map, buckets, i)                                             \
    ((Bucket *)(buckets + (i * BUCKET_SIZE(map))))
#define KEY_OF(bucket) ((char *)&(bucket)->data)
#define VALUE_OF(bucket, map) ((char *)&(bucket)->data + (map)->key_size)
#define MAP_MAX_LOAD 0.75

void hashmap_init(HashMap *map, hash_function *hash, eq_function *eq,
                  usize key_size, usize value_size)
{
    map->hash = hash;
    map->eq = eq;

    map->len = 0;

    map->key_size = key_size;
    map->value_size = value_size;

    map->capacity = 8;
    map->buckets = calloc(map->capacity, BUCKET_SIZE(map));
}

void hashmap_free(HashMap *map) { free(map->buckets); }

static Bucket *find_bucket(HashMap *map, char *buckets, usize capacity,
                           void *key, u64 hash)
{
    usize index = hash % capacity;
    Bucket *tombstone = NULL;
    for (;;)
    {
        Bucket *bucket = BUCKET_AT(map, buckets, index);

        if (!bucket->occupied)
        {
            if (!bucket->dead)
            {
                // empty bucket.
                return tombstone ? tombstone : bucket;
            }
            else
            {
                // we found a tombstone.
                if (!tombstone)
                    tombstone = bucket;
            }
        }
        else
        {
            bool is_eq = map->eq(KEY_OF(bucket), key, map->key_size);
            if (is_eq)
                // we found the key.
                return bucket;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(HashMap *map, usize new_capacity)
{
    char *new_buckets = calloc(new_capacity, BUCKET_SIZE(map));

    map->len = 0;
    for (u32 i = 0; i < map->capacity; i++)
    {
        Bucket *bucket = BUCKET_AT(map, map->buckets, i);
        if (!bucket->occupied)
            continue;

        Bucket *new_bucket = find_bucket(map, new_buckets, new_capacity,
                                         KEY_OF(bucket), bucket->hash);
        memcpy(new_bucket, bucket, BUCKET_SIZE(map));
        map->len++;
    }

    free(map->buckets);

    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

bool hashmap_insert(HashMap *map, void *key, void *value)
{
    if (map->len + 1 > map->capacity * MAP_MAX_LOAD)
    {
        usize new_capacity = map->capacity * 2;
        adjust_capacity(map, new_capacity);
    }

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, map->buckets, map->capacity, key, hash);
    bool is_new = !bucket->occupied;

    if (is_new)
    {
        if (!bucket->dead)
            map->len++;
        memcpy(KEY_OF(bucket), key, map->key_size);
        if (map->value_size > 0)
            memcpy(VALUE_OF(bucket, map), value, map->value_size);

        bucket->hash = hash;
        bucket->occupied = true;
        bucket->dead = false;
    }

    return is_new;
}

void *hashmap_get(HashMap *map, void *key)
{
    if (map->len == 0)
        return NULL;

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, map->buckets, map->capacity, key, hash);
    if (!bucket->occupied)
        return NULL;

    return VALUE_OF(bucket, map);
}

bool hashmap_remove(HashMap *map, void *key, void *value)
{
    if (map->len == 0)
        return false;

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, map->buckets, map->capacity, key, hash);
    if (!bucket->occupied)
        return false;

    if (value)
        memcpy(value, VALUE_OF(bucket, map), map->value_size);

    bucket->occupied = false;
    bucket->dead = true;

    return true;
}

bool hashmap_contains(HashMap *map, void *key)
{
    if (map->len == 0)
        return false;

    u64 hash = map->hash(key, map->key_size);
    Bucket *bucket = find_bucket(map, map->buckets, map->capacity, key, hash);
    return bucket->occupied;
}

void hashmap_clear(HashMap *map)
{
    memset(map->buckets, 0, map->capacity * BUCKET_SIZE(map));
    map->len = 0;
}

void hashmap_iter_init(HashMap *map, HashMapIter *iter)
{
    iter->map = map;
    iter->index = 0;
}

bool hashmap_iter_next(HashMapIter *iter, void **key, void **value)
{
    HashMap *map = iter->map;
    while (iter->index < map->capacity)
    {
        Bucket *bucket = BUCKET_AT(map, map->buckets, iter->index);
        iter->index++;
        if (bucket->occupied)
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