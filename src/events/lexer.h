#pragma once
#include "token.h"

int lexer_process(char *str, Token ***out_tokens, size_t *out_num_tokens, char out_err_msg[256]);

// returns the number of characters read from str
size_t lexer_next_token(char *str, Token *out_token, char out_err_msg[256]);
