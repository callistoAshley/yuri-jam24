#pragma once

#include "cglm/quat.h"
#include "core_types.h"

// general player state should be kept here, like inventory and puzzle flags

typedef struct
{
    Transform transform;
} Player;

static const Player PLAYER_INIT = {.transform = TRANSFORM_UNIT};
