#pragma once

#include "graphics/graphics.h"
#include "physics.h"

void physics_debug_draw(Physics *physics, Graphics *graphics, Camera raw_camera,
                        WGPURenderPassEncoder pass);
