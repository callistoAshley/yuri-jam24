#include "ini.h"

static IniSection *init_section(void)
{
    IniSection *section = malloc(sizeof(IniSection));
    section->name = malloc(1);
    section->pairs = linked_list_init();
    return section;
}

static IniPair *init_pair(void)
{
    IniPair *pair = malloc(sizeof(IniPair));
    pair->key = malloc(1);
    pair->value = malloc(1);
    return pair;
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\n' || c == '\r';
}

static Ini *parse_string(Ini *ini, char *str, size_t len, char out_err_msg[256])
{
#define PARSE_ERR(str) snprintf(out_err_msg, 256, "ini.c parse_string: " str), NULL // to be used in a return statement 
    IniSection *current_section = NULL;
    IniPair *current_pair = NULL;
    int write_idx = 0;
    bool escaped = false;
    enum { PARSE_NONE, PARSE_SECTION, PARSE_KEY, PARSE_VALUE, PARSE_COMMENT } parse_state = PARSE_NONE;

    for (int i = 0; i < (int)len; i++)
    {
        switch (parse_state)
        {
            case PARSE_NONE:
            {
                if (*str == '[' && !escaped)
                {
                    if (current_section) linked_list_append(ini->sections, current_section);
                    current_section = init_section();
                    parse_state = PARSE_SECTION;
                    write_idx = 0;
                }
                else if (*str == ';' && !escaped)
                {
                    parse_state = PARSE_COMMENT;
                }
                else if (!is_whitespace(*str))
                {
                    if (!current_section) return PARSE_ERR("stray pair (no section)");
                    if (current_pair) linked_list_append(current_section->pairs, current_pair);
                    current_pair = init_pair();
                    parse_state = PARSE_KEY;
                    write_idx = 0;
                    current_pair->key[write_idx++] = *str;
                }
                break;
            }
            case PARSE_SECTION:
            {
                if (*str == ']' && !escaped)
                {
                    current_section->name[write_idx] = '\0';
                    parse_state = PARSE_NONE;
                    write_idx = 0;
                    break;
                }
                current_section->name = realloc(current_section->name, write_idx + 2);
                PTR_ERRCHK(current_section->name, "ini.c parse_string: realloc failure.");
                current_section->name[write_idx++] = *str;
                break;
            }
            case PARSE_KEY:
            {
                if (*str == '=' && !escaped)
                {
                    current_pair->key[write_idx] = '\0';
                    write_idx = 0;
                    parse_state = PARSE_VALUE;
                    break;
                }
                current_pair->key = realloc(current_pair->key, write_idx + 2);
                PTR_ERRCHK(current_pair->key, "ini.c parse_string: realloc failure.");
                current_pair->key[write_idx++] = *str;
                break;
            }
            case PARSE_VALUE:
            {
                current_pair->value = realloc(current_pair->value, write_idx + 2);
                PTR_ERRCHK(current_pair->value, "ini.c parse_string: realloc failure.");
                current_pair->value[write_idx++] = *str;
                break;
            }
            default: break; // thank you compiler very compiler
        }

        str++;

        if (*str == '\n' && !escaped)
        {
            if (parse_state == PARSE_KEY)
            {
                return PARSE_ERR("syntax error: incomplete key");
            }
            else if (parse_state == PARSE_SECTION)
            {
                return PARSE_ERR("syntax error: incomplete section name");
            }
            else if (parse_state == PARSE_VALUE)
            {
                if (!current_section) return PARSE_ERR("syntax error: no current section");

                current_pair->key[write_idx] = '\0';
                linked_list_append(current_section->pairs, current_pair);
            }
            parse_state = PARSE_NONE;
        }

        if (*str == '\\' && !escaped) escaped = true;
        else if (escaped) escaped = false;
    }

    return ini;
#undef PARSE_ERR
}

Ini *ini_parse_string_n(const char *string, size_t string_len, char out_err_msg[256])
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
