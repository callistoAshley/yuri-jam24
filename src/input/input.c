#include "input.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

void input_init(Input *input)
{
    input->prev = 0;
    input->curr = 0;
    input->last_frame = SDL_GetTicksNS();
}

void input_start_frame(Input *input)
{
    input->prev = input->curr;
    u64 current_frame = SDL_GetTicksNS();
    input->delta = current_frame - input->last_frame;
    input->delta_seconds = SDL_NS_TO_SECONDS((f32)input->delta);
    input->last_frame = current_frame;
}

void input_process(SDL_Event *event, Input *input)
{
    bool key_is_down = event->type == SDL_EVENT_KEY_DOWN;
#define TOGGLE_BUTTON_IF_DOWN(button)                                          \
    if (key_is_down)                                                           \
        input->curr |= button;                                                 \
    else                                                                       \
        input->curr &= ~button;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        input->curr |= Button_Quit;
        break;
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key)
        {
        case SDLK_LEFT:
            TOGGLE_BUTTON_IF_DOWN(Button_Left);
            break;
        case SDLK_RIGHT:
            TOGGLE_BUTTON_IF_DOWN(Button_Right);
            break;
        case SDLK_UP:
            TOGGLE_BUTTON_IF_DOWN(Button_Up);
            break;
        case SDLK_DOWN:
            TOGGLE_BUTTON_IF_DOWN(Button_Down);
            break;
        case SDLK_SPACE:
            TOGGLE_BUTTON_IF_DOWN(Button_Jump);
            break;
        case SDLK_C:
            TOGGLE_BUTTON_IF_DOWN(Button_Crouch);
            break;
        case SDLK_ESCAPE:
            TOGGLE_BUTTON_IF_DOWN(Button_Quit);
            break;
        case SDLK_F11:
            TOGGLE_BUTTON_IF_DOWN(Button_Fullscreen);
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
