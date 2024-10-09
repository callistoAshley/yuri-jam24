#include "font.h"
#include "utility/log.h"
#include "utility/macros.h"
#include "utility/graphics.h"

void font_init(Font *font, const char *path, int size)
{
    log_info("loading font %s", path);
    font->font = TTF_OpenFont(path, size);
    SDL_PTR_ERRCHK(font->font, "failed to create font");
}

void font_free(Font *font) { TTF_CloseFont(font->font); }

void font_texture_size(Font *font, const char *text, int *w, int *h)
{
    TTF_SizeText(font->font, text, w, h);
}

SDL_Surface *font_render_surface(Font *font, const char *text, SDL_Color color)
{
    return TTF_RenderText_Solid(font->font, text, color);
}

void font_render_text_to(Font *font, WGPUTexture texture, const char *text,
                         SDL_Color color, WGPUResources *wgpu)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font->font, text, color);
    write_surface_to_texture(surface, texture, wgpu);
    SDL_DestroySurface(surface);
}

WGPUTexture font_render_text(Font *font, const char *text, SDL_Color color,
                             WGPUResources *wgpu)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font->font, text, color);
    WGPUTexture texture = texture_from_surface(
        surface, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
        wgpu);
    SDL_DestroySurface(surface);
    return texture;
}

void font_render_text_at(Font *font, u32 x, u32 y, WGPUTexture texture,
                         const char *text, SDL_Color color, WGPUResources *wgpu)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font->font, text, color);
    write_surface_to_texture_at(x, y, surface, texture, wgpu);
}
