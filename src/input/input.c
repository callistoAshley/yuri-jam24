#include "input.h"
#include <SDL3/SDL.h>

void input_new(Input *input)
{
    input->prev = 0;
    input->curr = 0;
}

void input_start_frame(Input *input)
{
    input->prev = input->curr;
    input->curr = 0;
}

void input_process(SDL_Event *event, Input *input)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        input->curr |= Button_Quit;
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key)
        {
        case SDLK_LEFT:
            input->curr |= Button_Left;
            break;
        case SDLK_RIGHT:
            input->curr |= Button_Right;
            break;
        case SDLK_UP:
            input->curr |= Button_Up;
            break;
        case SDLK_DOWN:
            input->curr |= Button_Down;
            break;
        case SDLK_SPACE:
            input->curr |= Button_Jump;
            break;
        case SDLK_ESCAPE:
            input->curr |= Button_Quit;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

bool input_is_down(Input *input, Button button) { return input->curr & button; }

bool input_is_pressed(Input *input, Button button)
{
    return (input->curr & button) && !(input->prev & button);
}

bool input_is_released(Input *input, Button button)
{
    return !(input->curr & button) && (input->prev & button);
}