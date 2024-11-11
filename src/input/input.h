#pragma once
#include "SDL3/SDL_keycode.h"
#include "sensible_nums.h"
#include "settings.h"
#include <SDL3/SDL_events.h>
#include <stdbool.h>

typedef enum
{
    Button_Left = 1 << 0,
    Button_Right = 1 << 1,
    Button_Up = 1 << 2,
    Button_Down = 1 << 3,

    Button_Jump = 1 << 4,
    Button_Cancel = 1 << 5,
    Button_Back = 1 << 6,
    Button_Quit = 1 << 7,
    Button_Interact = 1 << 8,
    Button_Inventory = 1 << 9,

    Button_Fullscreen = 1 << 10,
    Button_Refresh = 1 << 11,

    Button_MouseLeft = 1 << 12,
    Button_MouseRight = 1 << 13,
} Button;

typedef struct
{
    Button prev;
    Button curr;
    bool requested_quit, requested_fullscreen;

    SDL_Keycode last_pressed_key;
    bool key_has_pressed;

    i32 mouse_x, mouse_y;
    f32 mouse_scale_factor;
} Input;

void input_init(Input *input, SDL_Window *window);

void input_start_frame(Input *input);
void input_process(Input *input, SDL_Event *event, Settings *settings);

bool input_is_down(Input *input, Button button);
bool input_is_pressed(Input *input, Button button);
bool input_is_released(Input *input, Button button);
