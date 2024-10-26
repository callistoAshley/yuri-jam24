#include "autorun.h"
#include "characters/character.h"
#include "scenes/map.h"
#include "utility/log.h"

// these events do *nothing* except run an event when initialized
void *autorun_char_init(Resources *resources, struct MapScene *map_scene,
                        CharacterInitArgs *args)
{
    (void)map_scene;

    char *event_name = hashmap_get(args->metadata, "event");
    if (!event_name)
    {
        FATAL("Autorun characters do nothing without an attached event\n");
    }

    for (u32 i = 0; i < resources->event_count; i++)
    {
        Event event = resources->events[i];
        if (!strcmp(event.name, event_name))
        {
            VM *vm = malloc(sizeof(VM));
            vm_init(vm, event);
            return vm;
        }
    }
    FATAL("No such event `%s`\n", event_name);
}

void autorun_char_fixed_update(void **self, Resources *resources,
                               MapScene *map_scene)
{
    (void)map_scene;

    VM *vm = *self;
    if (vm)
    {
        bool finished = vm_execute(vm, resources);
        if (finished)
        {
            vm_free(vm);
            free(vm);
            *self = NULL;
        }
    }
}

void autorun_char_free(void *self, Resources *resources, MapScene *map_scene)
{
    (void)self;
    (void)resources;
    (void)map_scene;

    if (self)
    {
        VM *vm = self;
        vm_free(vm);
        free(vm);
    }
}
