#include "fonts.h"
#include "fonts/font.h"

void init_font_collection(FontCollection *collection, const char *path)
{
    font_init(&collection->small, path, SMALL_FONT_SIZE);
    font_init(&collection->medium, path, MEDIUM_FONT_SIZE);
    font_init(&collection->large, path, LARGE_FONT_SIZE);
}

void free_font_collection(FontCollection *collection)
{
    font_free(&collection->large);
    font_free(&collection->medium);
    font_free(&collection->small);
}

void fonts_init(Fonts *fonts)
{
    init_font_collection(&fonts->compaq, "assets/fonts/Mx437_Compaq_Port3.ttf");
    init_font_collection(&fonts->monogram, "assets/fonts/monogram.ttf");
}

void fonts_free(Fonts *fonts) { free_font_collection(&fonts->compaq); }
