#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windows/window-container.h"
#include "utility/macros.h"

typedef struct
{
    bool request_quit;

    Window *tmap_edit;

    WindowContainer *container;
} LevelEditor;

LevelEditor *lvledit_init(void);
void lvledit_update(LevelEditor *editor);
void lvledit_free(LevelEditor *editor);
