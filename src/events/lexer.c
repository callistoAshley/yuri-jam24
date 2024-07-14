#include "lexer.h"

int lexer_process(char *str, Token ***out_tokens, size_t *out_num_tokens, char out_err_msg[256])
{
    size_t num_read;
    Token token;
    while (num_read = lexer_next_token(str, &token, out_err_msg))
    {
        str += num_read;
        if (*str == '\0') break;
    }
}

size_t lexer_next_token(char *str, Token *out_token, char out_err_msg[256])
{
    size_t num_read = 0;
#define INCREMENT ++num_read, ++str

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
            case ';':
            {
                char name[256], arg[256];
                size_t name_len, arg_len;
                out_token->type = TOKEN_CMD;

                INCREMENT;

                // TODO: check if i exceeds the length of the buffer
                for (name_len = 0; *str != '('; name_len++, INCREMENT) name[name_len] = *str;
                name[name_len] = '\0';
                INCREMENT;

                for (arg_len = 0; *str != ')'; arg_len++, INCREMENT) arg[arg_len] = *str;
                arg[arg_len] = '\0';
                INCREMENT;

                strncpy(out_token->command.name, name, sizeof(out_token->command.name));
                strncpy(out_token->command.arg, name, sizeof(out_token->command.arg));
                return num_read;
            }
            default:
            {
                char text[256];
                out_token->type = TOKEN_TEXT;
                for (int i = 0; *str != '\n'; i++, INCREMENT)
                {
                    if (i >= 256)
                    {
                        snprintf(out_err_msg, sizeof(out_err_msg), "lexer_next_token: text exceeds maximum length of 256 characters");
                        return num_read;
                    }
                    text[i] = *str;
                }
                INCREMENT;
                strncpy(out_token->text, text, sizeof(out_token->text));
                return num_read;
            }
        }
    }
    
    return num_read;
}
