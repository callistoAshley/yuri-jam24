#include "item.h"
#include "scenes/map.h"
#include "ui/textbox.h"

static void on_use(Resources *resources)
{
    MapScene *scene = (MapScene *)resources->scene;

    textbox_display_text(&scene->textbox, resources, "i've been used");
}

const Item ITEMS[Item_Max] = {
    [Item_Test] = {"Test Item", "holy shit it works",
                   "assets/textures/test_item.png", on_use},
    [Item_TestOther] = {"Another test item", "e", "assets/textures/box.png",
                        NULL},
};
