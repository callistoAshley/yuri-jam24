#include "font.h"
#include "utility/log.h"
#include "utility/macros.h"

void font_init(Font *font, const char *path, int size)
{
    log_info("loading font %s", path);
    font->font = TTF_OpenFont(path, size);
    SDL_PTR_ERRCHK(font->font, "failed to create font");
}

void font_free(Font *font) { TTF_CloseFont(font->font); }
