#pragma once
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
    Button_Crouch = 1 << 5,
    Button_Back = 1 << 6,
    Button_Quit = 1 << 7,

    Button_Fullscreen = 1 << 8,

    Button_MouseLeft = 1 << 11,
    Button_MouseRight = 1 << 12,
} Button;

typedef struct
{
    Button prev;
    Button curr;
    bool requested_quit, requested_fullscreen;

    i32 mouse_x, mouse_y;

    u64 last_frame;
    u64 delta;
    f32 delta_seconds;
} Input;

void input_init(Input *input);

void input_start_frame(Input *input);
void input_process(SDL_Event *event, Input *input, Settings *settings);

bool input_is_down(Input *input, Button button);
bool input_is_pressed(Input *input, Button button);
bool input_is_released(Input *input, Button button);
