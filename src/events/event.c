#include "event.h"

Event *event_init(LinkedList *commands)
{
    Event *event;

    event = calloc(1, sizeof(Event));
    if (!event) return NULL;

    event->commands = commands;

    return event;
}

void event_free(Event *event)
{
    // go through each command in this event and free it if is_userdatum is false
    free(event);
}
