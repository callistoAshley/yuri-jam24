#include "hashset.h"
#include "sensible_nums.h"
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

void hashset_init(HashSet *set, hash_function *hash, eq_function *eq,
                  usize key_size)
{
    set->len = 0;
    set->key_size = key_size;

    // 8 is ok as a starting capacity (but we can always change this behavior
    // later)
    set->capacity = 8;

    usize bucket_size = sizeof(Bucket) + key_size;
    // keys are stored inline with the buckets
    set->buckets = calloc(set->capacity, bucket_size);

    set->hash = hash;
    set->eq = eq;
}

void hashset_free(HashSet *set) { free(set->buckets); }

#define TABLE_MAX_LOAD 0.75
// keys are stored inline with the buckets, so we need to do some pointer
// shenanigans
// only use this to read/write to a key!!! this pointer will always be non-null.
#define KEY_OF(b) ((void *)&(b)->key)

static Bucket *find_bucket(HashSet *set, void *key, u64 hash)
{
    usize index = hash % set->capacity;
    Bucket *tombstone = NULL;
    for (;;)
    {
        Bucket *bucket = &set->buckets[index];

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
        else if (set->eq(KEY_OF(bucket), key, set->key_size))
        {
            return bucket;
        }

        index = (index + 1) % set->capacity;
    }
}

static void adjust_set_capacity(HashSet *set, usize new_capacity)
{
    usize bucket_size = sizeof(Bucket) + set->key_size;
    // calloc zeroes out the memory, which is perfect
    Bucket *new_buckets = calloc(new_capacity, bucket_size);

    // we effectively have to rebuild the set from scratch
    set->len = 0;
    for (usize i = 0; i < set->capacity; i++)
    {
        Bucket *bucket = &set->buckets[i];
        if (!bucket->occupied)
            continue;

        Bucket *new_bucket = find_bucket(set, KEY_OF(bucket), bucket->hash);
        memcpy(new_bucket, bucket, bucket_size);
        new_bucket->occupied = true;
        set->len++;
    }

    free(set->buckets);
    set->buckets = new_buckets;
    set->capacity = new_capacity;
}

bool hashset_insert(HashSet *set, void *key)
{
    if (set->len + 1 > set->capacity * TABLE_MAX_LOAD)
    {
        usize new_capacity = set->capacity * 2;
        adjust_set_capacity(set, new_capacity);
    }

    u64 hash = set->hash(key, set->key_size);
    Bucket *bucket = find_bucket(set, key, hash);
    bool is_new = !bucket->occupied;

    if (is_new)
    {
        // only increment the length if the bucket was not dead.
        // this is because we're not actually filling out a brand new bucket,
        // just reusing a dead one.
        if (!bucket->dead)
            set->len++;
        bucket->hash = hash;
        bucket->occupied = true;
        bucket->dead = false;
        memcpy(KEY_OF(bucket), key, set->key_size);
    }

    return is_new;
}

bool hashset_remove(HashSet *set, void *key)
{
    if (set->len == 0)
        return false;

    u64 hash = set->hash(key, set->key_size);
    Bucket *bucket = find_bucket(set, key, hash);
    if (!bucket->occupied)
        return false;

    // mark the bucket as dead
    bucket->dead = true;
    bucket->occupied = false;
    set->len--;

    return true;
}

bool hashset_contains(HashSet *set, void *key)
{
    u64 hash = set->hash(key, set->key_size);
    Bucket *bucket = find_bucket(set, key, hash);
    return bucket->occupied;
}

void hashset_clear(HashSet *set)
{
    set->len = 0;
    usize bucket_size = sizeof(Bucket) + set->key_size;
    memset(set->buckets, 0, set->capacity * bucket_size);
}
