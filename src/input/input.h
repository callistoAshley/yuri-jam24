#pragma once
#include <SDL3/SDL_events.h>
#include <stdbool.h>

typedef enum
{
    Button_Left = 1 << 0,
    Button_Right = 1 << 1,
    Button_Up = 1 << 2,
    Button_Down = 1 << 3,

    Button_Jump = 1 << 4,
    Button_Quit = 1 << 5,
} Button;

typedef struct
{
    Button prev;
    Button curr;
} Input;

void input_new(Input *input);

void input_start_frame(Input *input);
void input_process(SDL_Event *event, Input *input);

bool input_is_down(Input *input, Button button);
bool input_is_pressed(Input *input, Button button);
bool input_is_released(Input *input, Button button);
