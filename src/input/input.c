#include "input.h"
#include "SDL3/SDL_keycode.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

void input_init(Input *input)
{
    input->prev = 0;
    input->curr = 0;
    input->last_frame = SDL_GetTicksNS();
    input->requested_quit = false;

    input->mouse_x = 0;
    input->mouse_y = 0;
}

void input_start_frame(Input *input)
{
    input->prev = input->curr;
    u64 current_frame = SDL_GetTicksNS();
    input->delta = current_frame - input->last_frame;
    input->delta_seconds = SDL_NS_TO_SECONDS((f32)input->delta);
    input->last_frame = current_frame;
}

void input_process(SDL_Event *event, Input *input, Settings *settings)
{
    bool button_is_down = event->type == SDL_EVENT_KEY_DOWN ||
                          event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
#define TOGGLE_BUTTON_IF_DOWN(button)                                          \
    if (button_is_down)                                                        \
        input->curr |= button;                                                 \
    else                                                                       \
        input->curr &= ~button;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        input->curr |= Button_Quit;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        input->mouse_x = event->motion.x;
        input->mouse_y = event->motion.y;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        switch (event->button.button)
        {
        case SDL_BUTTON_LEFT:
            TOGGLE_BUTTON_IF_DOWN(Button_MouseLeft);
            break;
        case SDL_BUTTON_RIGHT:
            TOGGLE_BUTTON_IF_DOWN(Button_MouseRight);
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
    {
        u32 key = event->key.key;
        if (key == settings->keybinds.up)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Up)
        }
        if (key == settings->keybinds.down)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Down)
        }
        if (key == settings->keybinds.left)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Left)
        }
        if (key == settings->keybinds.right)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Right)
        }
        if (key == settings->keybinds.jump)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Jump)
        }
        if (key == settings->keybinds.crouch)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Crouch)
        }
        if (key == settings->keybinds.back)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Back)
        }
        if (key == settings->keybinds.quit)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Quit)
        }
        if (key == SDLK_F11)
        {
            TOGGLE_BUTTON_IF_DOWN(Button_Fullscreen)
        }
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
