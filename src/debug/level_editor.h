#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/graphics.h"
#include "graphics/tilemap.h"
#include "utility/macros.h"
#include "windows/window_container.h"

typedef struct
{
    bool request_quit;

    Window *brush_wnd;

    WindowContainer *container;
    Graphics *graphics;
} LevelEditor;

LevelEditor *lvledit_init(Graphics *graphics, Tilemap *tilemap);
void lvledit_update(LevelEditor *editor);
void lvledit_free(LevelEditor *editor);
