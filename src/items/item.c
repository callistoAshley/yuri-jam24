#include "item.h"

const Item ITEMS[Item_Max] = {
    [Item_Test] = {"Test Item", "holy shit it works",
                   "assets/textures/test_item.png", NULL},
    [Item_TestOther] = {"Another test item", "e", "assets/textures/box.png",
                        NULL},
};
