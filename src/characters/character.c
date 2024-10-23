#include "character.h"
#include "characters/autorun.h"
#include "characters/basic.h"
#include "characters/rigidbody.h"

const CharacterInterface CHARACTERS[Char_Max] = {
    [Char_Basic] =
        {
            .name = "basic",
            .init_fn = basic_char_init,
            .update_fn = basic_char_update,
            .free_fn = basic_char_free,
        },
    [Char_Autorun] =
        {
            .name = "autorun",
            .init_fn = autorun_char_init,
            .update_fn = autorun_char_update,
            .free_fn = autorun_char_free,
        },
    [Char_RigidBody] =
        {
            .name = "rigidbody",
            .init_fn = rigidbody_char_init,
            .fixed_update_fn = rigidbody_char_fixed_update,
            .update_fn = rigidbody_char_update,
            .free_fn = rigidbody_char_free,
        },
};
