#include "scene.h"

void scene_change(SceneInterface new_scene, Resources *resources,
                  void *extra_args)
{
    resources->scene_interface.free(resources);
    new_scene.init(resources, extra_args);
    resources->scene_interface = new_scene;
}
