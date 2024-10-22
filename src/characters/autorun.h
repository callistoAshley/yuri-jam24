#pragma once
#include "character.h"
#include "graphics/graphics.h"
#include "graphics/quad_manager.h"
#include "graphics/tex_manager.h"
#include "graphics/transform_manager.h"

void autorun_char_init(void **out, Resources *resources,
                       struct MapScene *map_scene, Rect rect, HashMap *metadata,
                       void *extra_args);

void autorun_char_update(void *self, Resources *resources,
                         struct MapScene *map_scene);

void autorun_char_free(void *self, Resources *resources,
                       struct MapScene *map_scene);
