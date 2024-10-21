#include "value.h"
#include <string.h>

// less things are falsey than truthy
bool value_is_truthy(Value value) { return !value_is_falsey(value); }
bool value_is_falsey(Value value)
{
    return VAL_IS_FALSE(value) || VAL_IS_NONE(value);
}

bool value_is_eq(Value value, Value other)
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
}
