#include "interpreter.h"
#include "scenes/map.h"
#include "scenes/scene.h"

void interpreter_init(Interpreter *interpeter, Event *event)
{
    interpeter->event = event;
    interpeter->currently = Currently_Executing;
    interpeter->instruction = 0;
}

void interpreter_update(Interpreter *interpeter, Resources *resources)
{
    MapScene *scene = (MapScene *)*resources->current_scene;
    f32 delta = duration_as_secs(resources->time.current.delta);
    while (interpeter->instruction < interpeter->event->num_tokens)
    {
        switch (interpeter->currently)
        {
        case Currently_Waiting:
        {
            switch (interpeter->state.waiting.on)
            {
            case Waiting_OnText:
            {
                if (scene->textbox.open)
                    return;
                break;
            }
            case Waiting_OnTimer:
            {
                interpeter->state.waiting.data.timer -= delta;
                if (interpeter->state.waiting.data.timer)
                    return;
                break;
            }
            }
            // we finished waiting, now we move on to the next instruction
            interpeter->instruction++;
        }
        default:
            break;
        }

        Instruction *current =
            &interpeter->event->instructions[interpeter->instruction];
        switch (current->type)
        {
        case TOKEN_NONE:
            break;
        case TOKEN_TEXT:
            textbox_display_text(&scene->textbox, resources, current->text);
            interpeter->currently = Currently_Waiting;
            interpeter->state.waiting.on = Waiting_OnText;
            return;
        case TOKEN_CMD:
            break;
        }

        interpeter->instruction++;
    }
}
