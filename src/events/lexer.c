#include "lexer.h"
#include "utility/macros.h"
#include <string.h>

void lexer_init(Lexer *lexer, const char *src)
{
    lexer->start = src;
    lexer->current = src;
}

static char lexer_read(Lexer *lexer)
{
    char character = *lexer->current;
    lexer->current++;
    return character;
}

static bool lexer_eof(Lexer *lexer) { return *lexer->current == '\0'; }

static char lexer_peek(Lexer *lexer)
{
    char character = *lexer->current;
    return character;
}

static Token basic_token(TokenType type)
{
    Token token = {.type = type};
    return token;
}

static Token op_token(OpType op)
{
    Token token = {.type = Token_Op, .data.op = op};
    return token;
}

static bool lexer_match(Lexer *lexer, char expected)
{
    if (lexer_eof(lexer))
        return false;
    if (lexer_peek(lexer) != expected)
        return false;
    lexer_read(lexer);
    return true;
}

static void lexer_skip_whitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = lexer_peek(lexer);
        switch (c)
        {
        // comments aren't really whitepace but they're convenient to handle
        // here
        case '#':
        {
            // read until we find a newline
            while (lexer_peek(lexer) != '\n' && !lexer_eof(lexer))
                lexer_read(lexer);
            break;
        }
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            lexer_read(lexer);
            break;
        default:
            return;
        }
    }
}

static Token lexer_read_string(Lexer *lexer)
{
    // read until we hit closing quote or eof
    while (lexer_peek(lexer) != '"' && !lexer_eof(lexer))
    {
        lexer_read(lexer);
    }

    // throw an error if eof
    if (lexer_eof(lexer))
        FATAL("Unterminated event string!");

    // closing quote
    lexer_read(lexer);

    // subtract 2 for opening and closing quotes
    usize string_len = lexer->current - lexer->start - 2;
    // add 1 for null terminator
    char *string_data = malloc(string_len + 1);
    // add 1 to avoid first quote
    memcpy(string_data, lexer->start + 1, string_len);
    string_data[string_len] = '\0';

    Token token = {.type = Token_String, .data.string = string_data};
    return token;
}

static bool is_numeric(char c) { return c >= '0' && c <= '9'; }

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static Token lexer_read_number(Lexer *lexer)
{
    while (is_numeric(lexer_peek(lexer)))
        lexer_read(lexer);

    // we don't need to check if the thing after the . is numeric because we
    // don't support method calls. lmao
    bool is_float = false;
    // . means float
    if (lexer_peek(lexer) == '.')
    {
        is_float = true;
        // consume the '.'
        lexer_read(lexer);

        while (is_numeric(lexer_peek(lexer)))
            lexer_read(lexer);
    }

    // 64 digits is probably enough right? right.
    char digits[64] = {0};
    usize digits_len = lexer->current - lexer->start;
    // >= because of null terminator
    if (digits_len >= sizeof(digits))
        FATAL("Numeric literals longer than %lu characters are unsupported",
              sizeof(digits));

    // copy in digits
    memcpy(digits, lexer->start, digits_len);

    Token token;
    if (is_float)
    {
        token.type = Token_Float;
        token.data._float = atof(digits);
    }
    else
    {
        token.type = Token_Int;
        token.data._int = atoi(digits);
    }
    return token;
}

static Token lexer_read_text(Lexer *lexer)
{
    bool is_label = false;
    char c = lexer_peek(lexer);
    while (is_numeric(c) || is_alpha(c) || c == ':')
    {
        if (c == ':')
            is_label = true;
        lexer_read(lexer);
        c = lexer_peek(lexer);
    }

    usize text_len = lexer->current - lexer->start;

    Token token;

    // while we could do fancy tree shenanigans to figure out what keyword the
    // identifier is, i really couldn't be bothered
    if (!is_label)
    {
        if (!strncmp(lexer->start, "event", text_len))
        {
            token.type = Token_Event;
            return token;
        }
        if (!strncmp(lexer->start, "goto", text_len))
        {
            token.type = Token_Event;
            return token;
        }
        if (!strncmp(lexer->start, "if", text_len))
        {
            token.type = Token_If;
            return token;
        }
        if (!strncmp(lexer->start, "loop", text_len))
        {
            token.type = Token_Event;
            return token;
        }
        if (!strncmp(lexer->start, "true", text_len))
        {
            token.type = Token_True;
            return token;
        }
        if (!strncmp(lexer->start, "false", text_len))
        {
            token.type = Token_False;
            return token;
        }
        if (!strncmp(lexer->start, "none", text_len))
        {
            token.type = Token_None;
            return token;
        }
    }

    if (is_label)
        text_len--;

    char *text = malloc(text_len + 1);
    // copy in text
    memcpy(text, lexer->start, text_len);
    text[text_len] = '\0';

    if (is_label)
    {
        token.type = Token_Label;
        token.data.label = text;
    }
    else
    {
        token.type = Token_Ident;
        token.data.ident = text;
    }

    return token;
}

bool lexer_next(Lexer *lexer, Token *token)
{
    lexer_skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (lexer_eof(lexer))
        return false;

    char c = lexer_read(lexer);

    if (is_numeric(c))
    {
        *token = lexer_read_number(lexer);
        return true;
    }

    if (is_alpha(c))
    {
        *token = lexer_read_text(lexer);
        return true;
    }

    // basic tokens
    switch (c)
    {
    case '(':
        *token = basic_token(Token_ParenL);
        return true;
    case ')':
        *token = basic_token(Token_ParenR);
        return true;
    case '{':
        *token = basic_token(Token_BraceL);
        return true;
    case '}':
        *token = basic_token(Token_BraceR);
        return true;
    case ',':
        *token = basic_token(Token_Comma);
        return true;

    // bool ops
    case '&':
        *token = op_token(Op_And);
        return true;
    case '|':
        *token = op_token(Op_Or);
        return true;

    // math ops
    case '-':
        *token = op_token(Op_Minus);
        return true;
    case '+':
        *token = op_token(Op_Plus);
        return true;
    case '*':
        *token = op_token(Op_Mult);
        return true;
    case '/':
        *token = op_token(Op_Div);
        return true;
    case '%':
        *token = op_token(Op_Mod);
        return true;

    // 2-char ops
    case '!':
    {
        if (lexer_match(lexer, '='))
            *token = op_token(Op_NotEq);
        else
            *token = op_token(Op_Not);
        return true;
    }
    case '=':
    {
        if (lexer_match(lexer, '='))
            *token = op_token(Op_Equals);
        else
            *token = op_token(Op_Set);
        return true;
    }
    case '<':
    {
        if (lexer_match(lexer, '='))
            *token = op_token(Op_LessEq);
        else
            *token = op_token(Op_Less);
        return true;
    }
    case '>':
    {
        if (lexer_match(lexer, '='))
            *token = op_token(Op_GreaterEq);
        else
            *token = op_token(Op_Greater);
        return true;
    }

    // literals
    case '"':
        *token = lexer_read_string(lexer);
        return true;
    }

    // we couldn't handle this character. throw an error
    FATAL("Failed to handle character %c", c);
}
