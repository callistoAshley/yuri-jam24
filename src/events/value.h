#pragma once

#include "sensible_nums.h"
#include <stdbool.h>
#include <string.h>

typedef struct
{
    enum ValueType
    {
        Val_None = 0,
        Val_Int,
        Val_Float,
        Val_String,
        Val_True,
        Val_False,
    } type;
    union ValueData
    {
        i32 _int;
        f32 _float;
        char *string;
    } data;
} Value;

typedef enum ValueType ValueType;
typedef union ValueData ValueData;

#define VAL_IS_INT(value) ((value).type == Val_Int)
#define VAL_IS_FLOAT(value) ((value).type == Val_Float)
#define VAL_IS_STRING(value) ((value).type == Val_String)
#define VAL_IS_TRUE(value) ((value).type == Val_True)
#define VAL_IS_FALSE(value) ((value).type == Val_False)
#define VAL_IS_NONE(value) ((value).type == Val_None)

#define VAL_IS_NUMERIC(v) (VAL_IS_INT(v) || VAL_IS_FLOAT(v))

#define NONE_VAL ((Value){.type = Val_None})

#define TRUE_VAL ((Value){.type = Val_True})
#define FALSE_VAL ((Value){.type = Val_False})
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)

#define INT_VAL(v) ((Value){.type = Val_Int, .data._int = (v)})
#define FLOAT_VAL(v) ((Value){.type = Val_Float, .data._float = (v)})
#define STRING_VAL(v) ((Value){.type = Val_String, .data.string = (v)})

static inline bool value_is_falsey(Value value)
{
    return VAL_IS_FALSE(value) || VAL_IS_NONE(value);
}
static inline bool value_is_truthy(Value value)
{
    return !value_is_falsey(value);
}

static inline bool value_is_eq(Value value, Value other)
{
    bool is_numeric = VAL_IS_NUMERIC(value) && VAL_IS_NUMERIC(other);
    // if they're not the same type, return false
    // ...unless they're both numbers
    if (value.type != other.type && !is_numeric)
        return false;

    switch (value.type)
    {
    // if the two are the the same type, and they're one of these, they are
    // identical
    case Val_True:
    case Val_False:
    case Val_None:
        return true;
    case Val_Int:
    {
        if (VAL_IS_FLOAT(other))
            return value.data._int == other.data._float;
        else
            return value.data._int == other.data._int;
    }
    case Val_Float:
    {
        if (VAL_IS_FLOAT(other))
            return value.data._float == other.data._float;
        else
            return value.data._float == other.data._int;
    }
    case Val_String:
        return !strcmp(value.data.string, other.data.string);
    }

    return false;
}
