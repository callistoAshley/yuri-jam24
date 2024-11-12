#pragma once

#include "utility/macros.h"

typedef struct Resources Resources;

typedef enum
{
    Item_None = -1,
    Item_Test = 0,
    Item_TestOther,
    Item_Max,
} ItemType;

typedef void (*item_use_fn)(Resources *resources);

typedef struct
{
    const char *name;
    const char *description;
    const char *icon;

    NULLABLE item_use_fn on_use;
} Item;

extern const Item ITEMS[Item_Max];
