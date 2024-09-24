#pragma once

#include "font.h"

#define SMALL_FONT_SIZE 24
#define MEDIUM_FONT_SIZE 32
#define LARGE_FONT_SIZE 48

typedef struct
{
    // 24pt
    Font small;
    // 32pt
    Font medium;
    // 48pt
    Font large;
} FontCollection;

typedef struct
{
    FontCollection compaq;
} Fonts;

void fonts_init(Fonts *fonts);
void fonts_free(Fonts *fonts);
