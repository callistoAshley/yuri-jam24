#include "input.h"
#include "SDL3/SDL_keycode.h"
#include "utility/common_defines.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

void input_init(Input *input, SDL_Window *window)
{
    input->prev = 0;
    input->curr = 0;
    input->requested_quit = false;

    input->mouse_x = 0;
    input->mouse_y = 0;

    input->key_has_pressed = false;
    input->last_pressed_key = 0;

    i32 w = 0;
    SDL_GetWindowSizeInPixels(window, &w, NULL);
    input->mouse_scale_factor = (f32)UI_VIEW_WIDTH / w;
}

void input_start_frame(Input *input)
{
    input->prev = input->curr;
    input->key_has_pressed = false;
}

void input_process(Input *input, SDL_Event *event, Settings *settings)
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
    case SDL_EVENT_WINDOW_RESIZED:
        input->mouse_scale_factor = (f32)UI_VIEW_WIDTH / event->window.data1;
        break;
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
        input->last_pressed_key = key;
        input->key_has_pressed = true;
#define HANDLE_BUTTON(keybind, button)                                         \
    if (key == keybind)                                                        \
    {                                                                          \
        TOGGLE_BUTTON_IF_DOWN(button)                                          \
    }

        HANDLE_BUTTON(settings->keybinds.up, Button_Up);
        HANDLE_BUTTON(settings->keybinds.down, Button_Down);
        HANDLE_BUTTON(settings->keybinds.left, Button_Left);
        HANDLE_BUTTON(settings->keybinds.right, Button_Right);

        HANDLE_BUTTON(settings->keybinds.jump, Button_Jump);
        HANDLE_BUTTON(settings->keybinds.cancel, Button_Cancel);
        HANDLE_BUTTON(settings->keybinds.interact, Button_Interact);

        HANDLE_BUTTON(settings->keybinds.back, Button_Back);
        HANDLE_BUTTON(settings->keybinds.quit, Button_Quit);
        HANDLE_BUTTON(settings->keybinds.inventory, Button_Inventory);

        HANDLE_BUTTON(SDLK_F11, Button_Fullscreen);
        HANDLE_BUTTON(SDLK_F5, Button_Refresh);
    }
    break;
    default:
        break;
    }
}

bool input_is_down(Input *input, Button button) { return input->curr & button; }

bool input_did_press(Input *input, Button button)
{
    return (input->curr & button) && !(input->prev & button);
}

bool input_is_released(Input *input, Button button)
{
    return !(input->curr & button) && (input->prev & button);
}
