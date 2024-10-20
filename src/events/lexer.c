#include "lexer.h"
#include "utility/macros.h"
#include <stdio.h>
#include <string.h>

void lexer_init(Lexer *lexer, const char *src)
{
    lexer->start = src;
    lexer->current = src;
}

static char read(Lexer *lexer)
{
    char character = *lexer->current;
    lexer->current++;
    return character;
}

bool lexer_eof(Lexer *lexer) { return *lexer->current == '\0'; }

static char peek(Lexer *lexer)
{
    char character = *lexer->current;
    return character;
}

static Token basic_token(TokenType type)
{
    Token token = {.type = type};
    return token;
}

static bool match(Lexer *lexer, char expected)
{
    if (lexer_eof(lexer))
        return false;
    if (peek(lexer) != expected)
        return false;
    read(lexer);
    return true;
}

static void skip_whitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = peek(lexer);
        switch (c)
        {
        // comments aren't really whitepace but they're convenient to handle
        // here
        case '#':
        {
            // read until we find a newline
            while (peek(lexer) != '\n' && !lexer_eof(lexer))
                read(lexer);
            break;
        }
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            read(lexer);
            break;
        default:
            return;
        }
    }
}

static Token read_string(Lexer *lexer)
{
    // read until we hit closing quote or eof
    while (peek(lexer) != '"' && !lexer_eof(lexer))
    {
        read(lexer);
    }

    // throw an error if eof
    if (lexer_eof(lexer))
        FATAL("Unterminated event string!");

    // closing quote
    read(lexer);

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

static Token read_number(Lexer *lexer)
{
    while (is_numeric(peek(lexer)))
        read(lexer);

    // we don't need to check if the thing after the . is numeric because we
    // don't support method calls. lmao
    bool is_float = false;
    // . means float
    if (peek(lexer) == '.')
    {
        is_float = true;
        // consume the '.'
        read(lexer);

        while (is_numeric(peek(lexer)))
            read(lexer);
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

static Token read_text(Lexer *lexer)
{
    bool is_label = false;
    char c = peek(lexer);
    while (is_numeric(c) || is_alpha(c) || c == ':')
    {
        if (c == ':')
            is_label = true;
        read(lexer);
        c = peek(lexer);
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
        if (!strncmp(lexer->start, "else", text_len))
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
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (lexer_eof(lexer))
        return false;

    char c = read(lexer);

    if (is_numeric(c))
    {
        *token = read_number(lexer);
        return true;
    }

    if (is_alpha(c))
    {
        *token = read_text(lexer);
        return true;
    }

    // basic tokens
    switch (c)
    {
    case ';':
        *token = basic_token(Token_Semicolon);
        return true;

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
        *token = basic_token(Token_And);
        return true;
    case '|':
        *token = basic_token(Token_Or);
        return true;

    // math ops
    case '-':
        *token = basic_token(Token_Minus);
        return true;
    case '+':
        *token = basic_token(Token_Plus);
        return true;
    case '*':
        *token = basic_token(Token_Mult);
        return true;
    case '/':
        *token = basic_token(Token_Div);
        return true;
    case '%':
        *token = basic_token(Token_Mod);
        return true;

    // 2-char ops
    case '!':
    {
        if (match(lexer, '='))
            *token = basic_token(Token_NotEq);
        else
            *token = basic_token(Token_Not);
        return true;
    }
    case '=':
    {
        if (match(lexer, '='))
            *token = basic_token(Token_Equals);
        else
            *token = basic_token(Token_Set);
        return true;
    }
    case '<':
    {
        if (match(lexer, '='))
            *token = basic_token(Token_LessEq);
        else
            *token = basic_token(Token_Less);
        return true;
    }
    case '>':
    {
        if (match(lexer, '='))
            *token = basic_token(Token_GreaterEq);
        else
            *token = basic_token(Token_Greater);
        return true;
    }

    // literals
    case '"':
        *token = read_string(lexer);
        return true;
    }

    // we couldn't handle this character. throw an error
    FATAL("Failed to handle character %c", c);
}

void token_debug_printf(Token token)
{
    switch (token.type)
    {
    case Token_Semicolon:
        printf(";");
        break;
    case Token_Event:
        printf("event");
        break;
    case Token_Goto:
        printf("goto");
        break;
    case Token_If:
        printf("if");
        break;
    case Token_Else:
        printf("else");
        break;
    case Token_Loop:
        printf("loop");
        break;
    case Token_None:
        printf("none");
        break;
    case Token_True:
        printf("true");
        break;
    case Token_False:
        printf("false");
        break;
    case Token_Ident:
        printf("%s", token.data.ident);
        break;
    case Token_Label:
        printf("%s", token.data.label);
        break;
    case Token_BraceL:
        printf("{");
        break;
    case Token_BraceR:
        printf("}");
        break;
    case Token_ParenL:
        printf("(");
        break;
    case Token_ParenR:
        printf(")");
        break;
    case Token_Comma:
        printf(",");
        break;
    case Token_String:
        printf("'%s'", token.data.string);
        break;
    case Token_Int:
        printf("%d", token.data._int);
        break;
    case Token_Float:
        printf("%f", token.data._float);
        break;
    case Token_Set:
        printf("=");
        break;
    case Token_Equals:
        printf("==");
        break;
    case Token_NotEq:
        printf("!=");
        break;
    case Token_Less:
        printf("<");
        break;
    case Token_LessEq:
        printf("<=");
        break;
    case Token_Greater:
        printf(">");
        break;
    case Token_GreaterEq:
        printf(">=");
        break;
    case Token_Not:
        printf("!");
        break;
    case Token_And:
        printf("&");
        break;
    case Token_Or:
        printf("|");
        break;
    case Token_Plus:
        printf("+");
        break;
    case Token_Minus:
        printf("-");
        break;
    case Token_Mult:
        printf("*");
        break;
    case Token_Div:
        printf("/");
        break;
    case Token_Mod:
        printf("%%");
        break;
    }
}
