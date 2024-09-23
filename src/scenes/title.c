#include "title.h"

void title_scene_init(Scene **scene_data, Resources *resources)
{
    TitleScene *title_scene = malloc(sizeof(TitleScene));
    title_scene->type = Scene_Title;
    *scene_data = (Scene *)title_scene;
}

void title_scene_update(Scene *scene_data, Resources *resources) {}

void title_scene_free(Scene *scene_data, Resources *resources)
{
    free(scene_data);
}

const SceneInterface TITLE_SCENE = {
    .init = title_scene_init,
    .update = title_scene_update,
    .free = title_scene_free,
};
