#include "scene.h"

void scene_change(SceneInterface new_scene, Resources *resources,
                  void *extra_args)
{
    resources->current_scene_interface->free(*resources->current_scene,
                                             resources);
    new_scene.init(resources->current_scene, resources, extra_args);
    *resources->current_scene_interface = new_scene;
}
