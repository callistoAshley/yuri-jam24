#include "ini.h"

IniSection *init_section(void)
{
    IniSection *section = calloc(1, sizeof(IniSection));
    section->name = malloc(1);
    section->pairs = linked_list_init();
    return section;
}

IniPair *init_pair(void)
{
    IniPair *pair = calloc(1, sizeof(IniPair));
    pair->key = malloc(1);
    pair->value = malloc(1);
    return pair;
}

static Ini *parse_string(Ini *ini, const char *str, size_t len,
                         char out_err_msg[256])
{
    const char *end = str + len;
    IniSection *current_section = NULL;
    bool escaped = false;

    for (; str != end; str++)
    {
        switch (*str)
        {
        // skip whitespaces
        case '\n':
        case '\t':
        case '\r':
        case ' ':
            break;
        // new section
        case '[':
        {
            current_section = init_section();
            int i;
            str++;
            for (i = 0; (*str != ']' && !escaped) && str != end; str++, i++)
            {
                if ((escaped = *str == '\\' && !escaped))
                    break;
                PTR_ERRCHK((current_section->name =
                                realloc(current_section->name, i + 2)),
                           "realloc failure");
                current_section->name[i] = *str;
            }
            current_section->name[i] = '\0';
            linked_list_append(ini->sections, current_section);
            str++;
            break;
        }
        default:
        {
            if (!current_section)
            {
                snprintf(out_err_msg, 256, "pair defined outside of a section");
                ini_free(ini);
                return NULL;
            }
            IniPair *pair = init_pair();
            int i;
            for (i = 0; (*str != '=' && !escaped) && str != end; str++, i++)
            {
                if ((escaped = *str == '\\' && !escaped))
                    break;
                PTR_ERRCHK((pair->key = realloc(pair->key, i + 2)),
                           "realloc failure");
                pair->key[i] = *str;
            }
            pair->key[i] = '\0';
            str++;
            for (i = 0; (*str != '\n' && !escaped) && str != end; str++, i++)
            {
                if ((escaped = *str == '\\' && !escaped))
                    break;
                PTR_ERRCHK((pair->value = realloc(pair->value, i + 2)),
                           "realloc failure");
                pair->value[i] = *str;
            }
            pair->value[i] = '\0';
            linked_list_append(current_section->pairs, pair);
            break;
        }
        }
    }

    return ini;
}

Ini *ini_parse_string_n(const char *string, size_t string_len,
                        char out_err_msg[256])
{
    Ini *ini = calloc(1, sizeof(Ini));
    ini->sections = linked_list_init();
    return parse_string(ini, string, string_len, out_err_msg);
}

Ini *ini_parse_string(const char *string, char out_err_msg[256])
{
    return ini_parse_string_n(string, strlen(string), out_err_msg);
}

Ini *ini_parse_file(const char *path, char out_err_msg[256])
{
    char *file_string;
    long file_len;
    read_entire_file(path, &file_string, &file_len);
    return ini_parse_string_n(file_string, file_len, out_err_msg);
}

void ini_free(Ini *ini)
{
    for (int i = 0; i < ini->sections->len; i++)
    {
        IniSection *section = linked_list_at(ini->sections, i);
        for (int j = 0; j < section->pairs->len; j++)
        {
            IniPair *pair = linked_list_at(section->pairs, j);
            free(pair->key);
            free(pair->value);
            free(pair);
        }
        free(section->name);
        linked_list_free(section->pairs);
        free(section);
    }
    linked_list_free(ini->sections);
    free(ini);
}
