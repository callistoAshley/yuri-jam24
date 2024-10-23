#include "autorun.h"
#include "characters/character.h"
#include "scenes/map.h"
#include "utility/log.h"

// these events do *nothing* except run an event when initialized
void *autorun_char_init(Resources *resources, struct MapScene *map_scene,
                        CharacterInitArgs *args)
{

    char *event_name = hashmap_get(args->metadata, "event");
    if (!event_name)
    {
        log_warn("Autorun characters do nothing without an attached event\n");
        return NULL;
    }

    for (u32 i = 0; i < resources->event_count; i++)
    {
        Event event = resources->events[i];
        if (!strcmp(event.name, event_name))
        {
            VM vm;
            vm_init(&vm, event);
            vec_push(&map_scene->vms, &vm);
            return NULL;
        }
    }
    log_warn("No such event `%s`", event_name);

    return NULL;
}

void autorun_char_update(void *self, Resources *resources, MapScene *map_scene)
{
    (void)self;
    (void)resources;
    (void)map_scene;
}

void autorun_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)self;
    (void)resources;
    (void)map_scene;

    // the map_scene_free will handle all this for us
}
