#include "textbox.h"
#include "graphics/tex_manager.h"
#include "utility/common_defines.h"
#include "utility/graphics.h"
#include "utility/macros.h"
#include "webgpu.h"

#define INPUT_BUTTONS_DOWN(resources)                                          \
    (input_is_pressed(resources->input, Button_Interact) ||                    \
     input_is_pressed(resources->input, Button_Cancel))

void textbox_init(Textbox *textbox, Resources *resources)
{
    memset(textbox->text, 0, sizeof(textbox->text));
    textbox->text_idx = 0;
    textbox->text_type_time = 0.0f;
    textbox->typing = false;
    textbox->waiting_for_input = false;
    textbox->needs_remove_text = false;
    textbox->open = false;

    {
        TextureEntry *texture = texture_manager_load(
            &resources->graphics->texture_manager, TEXTURE_PATH("textbox.png"),
            &resources->graphics->wgpu);
        WGPUTexture wgpu_texture = texture_manager_get_texture(
            &resources->graphics->texture_manager, texture);

        Quad quad = {
            .rect = rect_from_size(
                (vec2s){.x = wgpuTextureGetWidth(wgpu_texture),
                        .y = wgpuTextureGetHeight(wgpu_texture)}),
            .tex_coords = RECT_UNIT_TEX_COORDS,
        };
        QuadEntry quad_entry =
            quad_manager_add(&resources->graphics->quad_manager, quad);

        Transform transform =
            transform_from_scale(VEC3_SPLAT((f32)WINDOW_SCALE / 2));
        TransformEntry transform_entry = transform_manager_add(
            &resources->graphics->transform_manager, transform);

        ui_sprite_init(&textbox->sprite, texture, transform_entry, quad_entry,
                       0.0f);
        textbox->sprite_entry =
            layer_add(&resources->graphics->ui_layers.middle, &textbox->sprite);
    }
}

void textbox_free(Textbox *textbox, Resources *resources)
{
    ui_sprite_free(&textbox->sprite, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.middle, textbox->sprite_entry);
}

static void update_text(Textbox *textbox, Resources *resources)
{
    SDL_Color color = {255, 255, 255, 255};
    char text[256];
    memset(text, 0, sizeof(text));
    strncpy(text, textbox->text, fmin(textbox->text_idx, 256));

    font_render_text_to(
        &resources->fonts->monogram.medium,
        texture_manager_get_texture(&resources->graphics->texture_manager,
                                    textbox->text_sprite.texture),
        text, color, &resources->graphics->wgpu);
}

static void process_escape_code(Textbox *textbox, Resources *resources)
{
    (void)resources;

    if (textbox->text[textbox->text_idx] == '\\')
    {
        switch (textbox->text[textbox->text_idx + 1])
        {
        case '.':
            textbox->text_type_time = -0.5f;
            break;
        }
        memmove(textbox->text + textbox->text_idx,
                textbox->text + textbox->text_idx + 2,
                strlen(textbox->text) - textbox->text_idx); // teehee
    }
}

static void remove_text(Textbox *textbox, Resources *resources)
{
    textbox->sprite.opacity = 0.0f;
    textbox->needs_remove_text = false;
    ui_sprite_free(&textbox->text_sprite, resources->graphics);
    layer_remove(&resources->graphics->ui_layers.foreground,
                 textbox->text_sprite_entry);
    memset(textbox->text, 0, sizeof(textbox->text));
}

void textbox_update(Textbox *textbox, Resources *resources)
{
    f32 delta = duration_as_secs(resources->time.real->time.delta);

    if (textbox->needs_remove_text)
        remove_text(textbox, resources);

    if (textbox->typing)
    {
        textbox->text_type_time += delta;
        if (textbox->text_type_time >= 0.01f)
        {
            textbox->text_idx++;
            textbox->text_type_time = 0.0f;
            update_text(textbox, resources);
            process_escape_code(textbox, resources);
        }
        if (INPUT_BUTTONS_DOWN(resources))
        {
            // skip typing animation
            while (textbox->text_idx < (int)strlen(textbox->text))
            {
                textbox->text_idx++;
                process_escape_code(textbox, resources);
            }
            update_text(textbox, resources);
        }
        if (textbox->text_idx >= (int)strlen(textbox->text))
        {
            textbox->typing = false;
            textbox->waiting_for_input = true;
        }
    }
    else if (textbox->waiting_for_input && INPUT_BUTTONS_DOWN(resources))
    {
        textbox->waiting_for_input = false;
        textbox->needs_remove_text = true; // hold off calling remove_text until
                                           // next frame, prevents flickering
        textbox->open = false;
    }
}

void textbox_display_text(Textbox *textbox, Resources *resources, char *text)
{
    if (*textbox->text)
    {
        remove_text(textbox, resources);
    }
    strncpy(textbox->text, text, sizeof(textbox->text));
    textbox->text_idx = 0;
    textbox->typing = true;
    textbox->waiting_for_input = false;
    textbox->sprite.opacity = 1.0f;
    textbox->text_type_time = 0.0f;
    textbox->open = true;

    // render the text with an invisible colour to get its width and height
    SDL_Color color = {0, 0, 0, 0};
    WGPUTexture texture =
        font_render_text(&resources->fonts->monogram.medium, textbox->text,
                         color, &resources->graphics->wgpu);

    u32 width = wgpuTextureGetWidth(texture),
        height = wgpuTextureGetHeight(texture);
    Transform transform = transform_from_pos((vec3s){.x = 24, .y = 8});
    TransformEntry transform_entry = transform_manager_add(
        &resources->graphics->transform_manager, transform);

    Quad quad = {
        .rect = rect_from_size((vec2s){.x = width, .y = height}),
        .tex_coords = RECT_UNIT_TEX_COORDS,
    };
    QuadEntry quad_entry =
        quad_manager_add(&resources->graphics->quad_manager, quad);

    ui_sprite_init(
        &textbox->text_sprite,
        texture_manager_register(&resources->graphics->texture_manager, texture,
                                 "textbox_text_sprite"),
        transform_entry, quad_entry, 1.0f);
    textbox->text_sprite_entry = layer_add(
        &resources->graphics->ui_layers.foreground, &textbox->text_sprite);
}
