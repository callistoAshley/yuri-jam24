#include "lexer.h"

size_t lexer_next_token(char *str, Instruction *out_token,
                        char out_err_msg[256])
{
#define INCREMENT ++num_read, ++str
    size_t num_read = 0;

    out_token->type = TOKEN_NONE;

    while (*str)
    {
        switch (*str)
        {
        // skip whitespace characters
        case ' ':
        case '\t':
        case '\n':
        {
            INCREMENT;
            break;
        }
        // comments
        case '#':
        {
            while (*str != '\n' && *str)
                INCREMENT;
            break;
        }
        case ';':
        {
            char name[256], arg[256];
            size_t name_len, arg_len;
            out_token->type = TOKEN_CMD;

            INCREMENT;

            // TODO: check if i exceeds the length of the buffer
            for (name_len = 0; *str != '('; name_len++, INCREMENT)
                name[name_len] = *str;
            name[name_len] = '\0';
            INCREMENT;

            for (arg_len = 0; *str != ')'; arg_len++, INCREMENT)
                arg[arg_len] = *str;
            arg[arg_len] = '\0';
            INCREMENT;

            strncpy(out_token->command.name, name,
                    sizeof(out_token->command.name));
            strncpy(out_token->command.arg, arg,
                    sizeof(out_token->command.arg));
            return num_read;
        }
        default:
        {
            char text[256];
            int text_len;
            out_token->type = TOKEN_TEXT;
            for (text_len = 0; *str != '\n' && *str; text_len++, INCREMENT)
            {
                if (text_len >= 256)
                {
                    snprintf(out_err_msg, 256,
                             "lexer_next_token: text exceeds maximum length of "
                             "256 characters");
                    return num_read;
                }
                text[text_len] = *str;
            }
            INCREMENT;
            text[text_len] = '\0';
            strncpy(out_token->text, text, sizeof(out_token->text));
            return num_read;
        }
        }
    }

    return num_read;
}
