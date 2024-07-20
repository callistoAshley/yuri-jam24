#pragma once
#include "sensible_nums.h"

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
    usize len, cap, ele_size;
    // if we had this as void*, because of C shenanigans
    // manipulating void* is only allowed in GCC compatible compilers
    char *data;
} vec;

typedef void (*vec_free_fn)(usize, void *);

// initializes a vec with the given element size
void vec_init(vec *v, usize ele_size);
void vec_init_with_capacity(vec *v, usize ele_size, usize cap);
// returns a copy of the given vec. the data is copied, so the original vec can
// be freed or modified without affecting the copy.
vec vec_dup(vec *v);

// frees the memory used by the vec
void vec_free(vec *v);
// frees the memory used by the vec, and calls free_fn on each element.
void vec_free_with(vec *v, vec_free_fn free_fn);

// returns a pointer to the element at the given index, or NULL if the index is
// out of bounds
void *vec_get(vec *v, usize index);

void vec_clear(vec *v);
void vec_clear_with(vec *v, vec_free_fn free_fn);

// resizes the vec to the given capacity.
void vec_resize(vec *v, usize new_cap);
// resizes the vec to the given capacity, and calls free_fn on each element that
// exists beyond the new capacity.
void vec_resize_with(vec *v, usize new_cap, vec_free_fn free_fn);

// inserts an element at the given index. if the index is == len, the element is
// appended to the end of the vec. if the index is out of bounds, nothing
// happens. otherwise, all elements after the index are shifted to the right.
void vec_insert(vec *v, usize index, void *elem);
// removes the element at the given index. if the index is out of bounds,
// nothing happens. if the index is < len - 1, all elements after the index are
// shifted to the left. the removed element is copied into the elem pointer, if
// it is not NULL.
void vec_remove(vec *v, usize index, void *elem);

// appends an element to the end of the vec. if the vec is full, it is resized.
void vec_push(vec *v, void *elem);
// removes the last element from the vec. if the vec is empty, nothing happens.
void vec_pop(vec *v, void *elem);
