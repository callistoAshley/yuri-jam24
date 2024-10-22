#include "character.h"
#include "characters/autorun.h"
#include "characters/basic.h"

const CharacterInterface CHARACTERS[Char_Max] = {
    [Char_Basic] =
        {
            .name = "basic",
            .type = Char_Basic,
            .init_fn = basic_char_init,
            .update_fn = basic_char_update,
            .free_fn = basic_char_free,
        },
    [Char_Autorun] =
        {
            .name = "autorun",
            .type = Char_Autorun,
            .init_fn = autorun_char_init,
            .update_fn = autorun_char_update,
            .free_fn = autorun_char_free,
        },
};
