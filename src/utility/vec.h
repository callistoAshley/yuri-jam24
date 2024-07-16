#pragma once
#include <stddef.h>

// a growable, resizable array.
// the data is stored in a contiguous block of memory, and the array is resized
// when its length exceeds its capacity.
//
// array capacity is doubled when it is full, and never shrinks unless
// vec_resize is called.
//
// NOT TYPE SAFE!!!
typedef struct
{
    size_t len, cap, ele_size;
    // if we had this as void*, because of C shenanigans
    // manipulating void* is only allowed in GCC compatible compilers
    char *data;
} vec;

typedef void (*vec_free_fn)(size_t, void *);

void vec_init(vec *v, size_t ele_size);
void vec_free(vec *v);
void vec_free_with(vec *v, vec_free_fn free_fn);

void *vec_get(vec *v, size_t index);

void vec_resize_with(vec *v, size_t new_cap, vec_free_fn free_fn);
void vec_resize(vec *v, size_t new_cap);
void vec_insert(vec *v, size_t index, void *elem);
void vec_remove(vec *v, size_t index, void *elem);

void vec_push(vec *v, void *elem);
void vec_pop(vec *v, void *elem);
