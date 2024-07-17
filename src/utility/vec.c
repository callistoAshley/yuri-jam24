#include <stdlib.h>
#include <string.h>
#include "vec.h"

#define VEC_GROWTH_FACTOR 2
#define INITIAL_CAP 4

void vec_init(vec *v, usize ele_size)
{
    v->len = 0;
    v->cap = INITIAL_CAP;
    v->ele_size = ele_size;
    v->data = malloc(v->cap * ele_size);
}

vec vec_dup(vec *v)
{
    vec new_vec;
    new_vec.len = v->len;
    new_vec.cap = v->cap;
    new_vec.ele_size = v->ele_size;
    new_vec.data = malloc(v->cap * v->ele_size);
    memcpy(new_vec.data, v->data, v->len * v->ele_size);
    return new_vec;
}

void vec_free(vec *v) { free(v->data); }
void vec_free_with(vec *v, vec_free_fn free_fn)
{
    for (usize i = 0; i < v->len; i++)
    {
        free_fn(v->ele_size, vec_get(v, i));
    }
    vec_free(v);
}

void *vec_get(vec *v, usize index)
{
    if (index >= v->len)
    {
        return NULL;
    }
    return v->data + index * v->ele_size;
}

void vec_resize(vec *v, usize new_cap)
{
    v->data = realloc(v->data, new_cap * v->ele_size);
    v->cap = new_cap;
}
void vec_resize_with(vec *v, usize new_cap, vec_free_fn free_fn)
{
    if (new_cap < v->len)
    {
        for (usize i = new_cap; i < v->len; i++)
        {
            free_fn(v->ele_size, vec_get(v, i));
        }
    }
    vec_resize(v, new_cap);
}

void vec_insert(vec *v, usize index, void *elem)
{
    // resize if necessary
    if (v->len == v->cap)
    {
        vec_resize(v, v->cap * VEC_GROWTH_FACTOR);
    }
    // if the index is inside the inner bounds of the array, shift everything to
    // the right
    if (index < v->len)
    {
        // shift everything to the right using memmove
        memmove(v->data + (index + 1) * v->ele_size,
                v->data + index * v->ele_size, (v->len - index) * v->ele_size);
    }
    // copy the element into the array
    memcpy(v->data + index * v->ele_size, elem, v->ele_size);
    v->len++;
}

void vec_remove(vec *v, usize index, void *elem)
{
    // if the index is outside the bounds of the array, return
    if (index >= v->len)
    {
        return;
    }
    // copy the element into the elem pointer
    if (elem)
        memcpy(elem, v->data + index * v->ele_size, v->ele_size);
    // if the index is inside the inner bounds of the array, shift everything to
    // the left
    if (index < v->len)
    {
        // shift everything to the left using memmove
        memmove(v->data + index * v->ele_size,
                v->data + (index + 1) * v->ele_size,
                (v->len - index) * v->ele_size);
    }
    v->len--;
}

void vec_push(vec *v, void *elem) { vec_insert(v, v->len, elem); }
void vec_pop(vec *v, void *elem) { vec_remove(v, v->len - 1, elem); }
