#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "token.h"

// returns the number of characters read from str
size_t lexer_next_token(char *str, Token *out_token, char out_err_msg[256]);
