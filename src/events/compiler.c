#include "compiler.h"
#include "events/lexer.h"
#include "events/instruction.h"
#include "utility/macros.h"
#include "utility/vec.h"

void compiler_init(Compiler *compiler, const char *source)
{
    lexer_init(&compiler->lexer, source);
}

// quite a lot of this is copied from
// https://craftinginterpreters.com/compiling-expressions.html#a-pratt-parser

static void advance(Compiler *compiler)
{
    compiler->previous = compiler->current;
    lexer_next(&compiler->lexer, &compiler->current);
}

static void emit(Compiler *compiler, Instruction instruction)
{
    vec_push(&compiler->instructions, &instruction);
}

static void emit_basic(Compiler *compiler, InstructionCode code)
{
    Instruction instruction = {.code = code};
    emit(compiler, instruction);
}

// emits an integer, assuming the previous consumed token was an int.
// it is undefined behaviour if this is not the case.
static void emit_int(Compiler *compiler)
{
    Instruction instruction = {
        .code = Code_Int,
        .data._int = compiler->previous.data._int,
    };
    emit(compiler, instruction);
}

// advances the compiler, and throws an error if the next token did not match
// the expected token type.
static void consume(Compiler *compiler, TokenType expected, const char *err)
{
    if (compiler->current.type == expected)
    {
        advance(compiler);
        return;
    }

    FATAL_ERR_MSG("%s, got ", err);
    token_debug_printf(compiler->current);
    printf("\n");
    exit(1);
}

typedef enum
{
    Prec_None,
    Prec_Set,     // =
    Prec_Or,      // |
    Prec_And,     // &
    Prec_Equals,  // ==
    Prec_Compare, // <, >, <=, >=
    Prec_Term,    // +, -
    Prec_Factor,  // *, /, %
    Prec_Unary,   // !, -
    Prec_Call,    // ()
    Prec_Primary
} Precendence;

typedef void (*ParseFn)(Compiler *compiler);
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precendence precedence;
} ParseRule;

static void expression(Compiler *compiler);
static const ParseRule *get_rule(TokenType type);
static void parse_precedence(Compiler *compiler, Precendence precedence);

// assumes the initial ( was consumed
static void grouping(Compiler *compiler)
{
    expression(compiler);
    consume(compiler, Token_ParenR, "Expected )");
}

static void binary(Compiler *compiler)
{
    TokenType op = compiler->previous.type;
    const ParseRule *rule = get_rule(op);
    parse_precedence(compiler, rule->precedence + 1);

    switch (op)
    {
    case Token_Plus: // +
        emit_basic(compiler, Code_Add);
        break;
    case Token_Minus: // -
        emit_basic(compiler, Code_Sub);
        break;
    case Token_Mult: // *
        emit_basic(compiler, Code_Mul);
        break;
    case Token_Div: // /
        emit_basic(compiler, Code_Div);
        break;
    case Token_Mod: // %
        emit_basic(compiler, Code_Mod);
        break;
    // should be unreachable
    default:
        return;
    }
}

static void unary(Compiler *compiler)
{
    TokenType op_type = compiler->previous.type;

    // compile the operand
    parse_precedence(compiler, Prec_Unary);

    switch (op_type)
    {
    case Token_Minus:
        emit_basic(compiler, Code_Negate);
        break;
    case Token_Not:
        emit_basic(compiler, Code_Not);
        break;
    // should be unreachable
    default:
        return;
    }
}

#define NULL_RULE                                                              \
    {                                                                          \
        NULL, NULL, Prec_None                                                  \
    }
const ParseRule rules[] = {
    [Token_Semicolon] = NULL_RULE,
    // keywords
    [Token_Event] = NULL_RULE,
    [Token_Goto] = NULL_RULE,
    [Token_If] = NULL_RULE,
    [Token_Else] = NULL_RULE,
    [Token_Loop] = NULL_RULE,
    // keyword values
    [Token_None] = NULL_RULE,
    [Token_True] = NULL_RULE,
    [Token_False] = NULL_RULE,

    // special
    [Token_Ident] = NULL_RULE,
    [Token_Label] = NULL_RULE,

    // braces
    [Token_BraceL] = NULL_RULE,
    [Token_BraceR] = NULL_RULE,

    // command calls
    [Token_ParenL] = {grouping, NULL, Prec_None},
    [Token_ParenR] = NULL_RULE,
    [Token_Comma] = NULL_RULE,

    // literals
    [Token_String] = NULL_RULE,
    [Token_Int] = {emit_int, NULL, Prec_None},
    [Token_Float] = NULL_RULE,

    [Token_Set] = NULL_RULE,

    // equality
    [Token_Equals] = NULL_RULE,
    [Token_NotEq] = NULL_RULE,
    [Token_Less] = NULL_RULE,
    [Token_LessEq] = NULL_RULE,
    [Token_Greater] = NULL_RULE,
    [Token_GreaterEq] = NULL_RULE,

    // boolean logic
    [Token_Not] = NULL_RULE,
    [Token_And] = NULL_RULE,
    [Token_Or] = NULL_RULE,

    [Token_Plus] = {NULL, binary, Prec_Term},
    [Token_Minus] = {unary, binary, Prec_Term},
    [Token_Mult] = {NULL, binary, Prec_Factor},
    [Token_Div] = {NULL, binary, Prec_Factor},
    [Token_Mod] = {NULL, binary, Prec_Factor},
};

static const ParseRule *get_rule(TokenType type) { return &rules[type]; }

static void parse_precedence(Compiler *compiler, Precendence precedence)
{
    advance(compiler);
    const ParseRule *rule = get_rule(compiler->previous.type);
    if (rule->prefix == NULL)
        FATAL("Expected expression");

    rule->prefix(compiler);

    while (precedence <= get_rule(compiler->current.type)->precedence)
    {
        advance(compiler);
        rule = get_rule(compiler->previous.type);
        rule->infix(compiler);
    }
}

static void expression(Compiler *compiler)
{
    parse_precedence(compiler, Prec_Set);
}

bool compiler_compile(Compiler *compiler, Event *event)
{
    if (lexer_eof(&compiler->lexer))
        return false;

    vec_init(&compiler->instructions, sizeof(Instruction));

    advance(compiler);
    consume(compiler, Token_Event, "Expected event definition");
    consume(compiler, Token_String, "Expected event name");
    event->name = compiler->previous.data.string;
    consume(compiler, Token_BraceL, "Expected {");

    expression(compiler);

    consume(compiler, Token_BraceR, "Expected }");

    event->instructions_len = compiler->instructions.len;
    event->instructions = (Instruction *)compiler->instructions.data;

    return true;
}
