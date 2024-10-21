#pragma once

#include "sensible_nums.h"
#include <stdbool.h>

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

bool value_is_truthy(Value value);
bool value_is_falsey(Value value);

bool value_is_eq(Value value, Value other);

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
