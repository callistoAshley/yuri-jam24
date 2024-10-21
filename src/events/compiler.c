#include "compiler.h"
#include "events/lexer.h"
#include "events/instruction.h"
#include "utility/macros.h"
#include "utility/vec.h"
#include <string.h>

void compiler_init(Compiler *compiler, const char *source)
{
    lexer_init(&compiler->lexer, source);
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

typedef void (*ParseFn)(Compiler *compiler, bool can_assign);
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precendence precedence;
} ParseRule;

static void expression(Compiler *compiler);
// we don't support defining things (aside from
// labels, but those are special anyway) so we
// don't need to handle declarations
static void statement(Compiler *compiler);
static const ParseRule *get_rule(TokenType type);
static void parse_precedence(Compiler *compiler, Precendence precedence);

// quite a lot of this is copied from
// https://craftinginterpreters.com/compiling-expressions.html#a-pratt-parser

// TODO handle EOF
static void advance(Compiler *compiler)
{
    compiler->previous = compiler->current;
    lexer_next(&compiler->lexer, &compiler->current);
}

static bool check(Compiler *compiler, TokenType type)
{
    return compiler->current.type == type;
}

static bool match(Compiler *compiler, TokenType type)
{
    if (!check(compiler, type))
        return false;
    advance(compiler);
    return true;
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

static void emit(Compiler *compiler, Instruction instruction)
{
    vec_push(&compiler->instructions, &instruction);
}

static void emit_basic(Compiler *compiler, InstructionCode code)
{
    Instruction instruction = {.code = code};
    emit(compiler, instruction);
}

// will emit a jump. callee will need to fill in the position!
static u32 emit_unknown_jump(Compiler *compiler, InstructionCode code)
{
    u32 current_instruction = compiler->instructions.len;
    Instruction instruction = {.code = code, .data.instruction = UINT32_MAX};
    emit(compiler, instruction);
    return current_instruction;
}

static void patch_jump(Compiler *compiler, u32 offset)
{
    u32 jump_pos = compiler->instructions.len;

    Instruction *instruction = vec_get(&compiler->instructions, offset);
    instruction->data.instruction = jump_pos;
}

static void emit_jump(Compiler *compiler, InstructionCode code, u32 position)
{
    Instruction instruction = {.code = code, .data.instruction = position};
    emit(compiler, instruction);
}

// emits an integer, assuming the previous consumed token was an int.
// it is undefined behaviour if this is not the case.
static void emit_int(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    Instruction instruction = {
        .code = Code_Int,
        .data._int = compiler->previous.data._int,
    };
    emit(compiler, instruction);
}

static void emit_float(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    Instruction instruction = {
        .code = Code_Float,
        .data._float = compiler->previous.data._float,
    };
    emit(compiler, instruction);
}

static void emit_string(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    Instruction instruction = {
        .code = Code_String,
        .data.string = compiler->previous.data.string,
    };
    emit(compiler, instruction);
}

static void emit_keyword_literal(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    switch (compiler->previous.type)
    {
    case Token_False:
        emit_basic(compiler, Code_False);
        break;
    case Token_True:
        emit_basic(compiler, Code_True);
        break;
    case Token_None:
        emit_basic(compiler, Code_None);
        break;
    default:
        return; // unreachable
    }
}

// TODO free variable name
static u32 get_or_insert_variable(Compiler *compiler, const char *name)
{
    for (u32 i = 0; i < compiler->variables.len; i++)
    {
        char *var = *(char **)vec_get(&compiler->variables, i);
        if (!strcmp(name, var))
            return i;
    }

    // looks like this variable hasn't been used yet.
    // push it to the array and return the slot
    u32 slot = compiler->variables.len;
    vec_push(&compiler->variables, &name);
    return slot;
}

static void variable(Compiler *compiler, bool can_assign)
{
    char *variable = compiler->previous.data.ident;
    u32 slot = get_or_insert_variable(compiler, variable);

    Instruction instruction = {.data.slot = slot};
    if (can_assign && match(compiler, Token_Set))
    {
        instruction.code = Code_Set;
        expression(compiler);
    }
    else
    {
        instruction.code = Code_Fetch;
    }
    emit(compiler, instruction);
}

// assumes the initial ( was consumed
static void grouping(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    expression(compiler);
    consume(compiler, Token_ParenR, "Expected )");
}

static void binary(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    TokenType op = compiler->previous.type;
    const ParseRule *rule = get_rule(op);
    parse_precedence(compiler, rule->precedence + 1);

    switch (op)
    {
    // math ops
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

    // comparison ops
    case Token_Eq: // ==
        emit_basic(compiler, Code_Eq);
        break;
    case Token_NotEq: // !=
        emit_basic(compiler, Code_NotEq);
        break;
    case Token_Greater: // >
        emit_basic(compiler, Code_Greater);
        break;
    case Token_GreaterEq: // >=
        emit_basic(compiler, Code_GreaterEq);
        break;
    case Token_Less: // <
        emit_basic(compiler, Code_Less);
        break;
    case Token_LessEq: // <=
        emit_basic(compiler, Code_LessEq);
        break;

    // should be unreachable
    default:
        return;
    }
}

static void unary(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
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
    [Token_None] = {emit_keyword_literal, NULL, Prec_None},
    [Token_True] = {emit_keyword_literal, NULL, Prec_None},
    [Token_False] = {emit_keyword_literal, NULL, Prec_None},

    // special
    [Token_Ident] = {variable, NULL, Prec_None},
    [Token_Label] = NULL_RULE,

    // braces
    [Token_BraceL] = NULL_RULE,
    [Token_BraceR] = NULL_RULE,

    // command calls
    [Token_ParenL] = {grouping, NULL, Prec_None},
    [Token_ParenR] = NULL_RULE,
    [Token_Comma] = NULL_RULE,

    // literals
    [Token_String] = {emit_string, NULL, Prec_None},
    [Token_Int] = {emit_int, NULL, Prec_None},
    [Token_Float] = {emit_float, NULL, Prec_None},

    [Token_Set] = NULL_RULE,

    // equality
    [Token_Eq] = {NULL, binary, Prec_Equals},
    [Token_NotEq] = {NULL, binary, Prec_Equals},
    [Token_Less] = {NULL, binary, Prec_Compare},
    [Token_LessEq] = {NULL, binary, Prec_Compare},
    [Token_Greater] = {NULL, binary, Prec_Compare},
    [Token_GreaterEq] = {NULL, binary, Prec_Compare},

    // boolean logic
    [Token_Not] = {unary, NULL, Prec_Term},
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
        FATAL("Expected expression\n");

    bool can_assign = precedence <= Prec_Set;
    rule->prefix(compiler, can_assign);

    while (precedence <= get_rule(compiler->current.type)->precedence)
    {
        advance(compiler);
        rule = get_rule(compiler->previous.type);
        rule->infix(compiler, can_assign);
    }

    if (can_assign && match(compiler, Token_Set))
    {
        FATAL("Invalid assignment target");
    }
}

static void expression(Compiler *compiler)
{
    parse_precedence(compiler, Prec_Set);
}

static void block(Compiler *compiler)
{
    while (!check(compiler, Token_BraceR) && !lexer_eof(&compiler->lexer))
    {
        statement(compiler);
    }

    consume(compiler, Token_BraceR, "Expected }");
}

static void expression_statement(Compiler *compiler)
{
    expression(compiler);
    consume(compiler, Token_Semicolon, "Expected ; after expression");
    emit_basic(compiler, Code_Pop);
}

static void if_statement(Compiler *compiler)
{
    // we don't do the () around ifs, and ifs always have to have a block
    expression(compiler);
    consume(compiler, Token_BraceL, "Expected {");

    // skip over if branch if condition was false
    u32 if_jump = emit_unknown_jump(compiler, Code_GotoIf);
    emit_basic(compiler, Code_Pop); // pop condition on if
    block(compiler);

    u32 else_jump = emit_unknown_jump(compiler, Code_Goto);

    // patch after the else jump to skip over it if the condition was true
    patch_jump(compiler, if_jump);
    emit_basic(compiler, Code_Pop); // pop condition on else

    if (match(compiler, Token_Else))
    {
        // if it's an else if, compile an if
        if (match(compiler, Token_If))
        {
            if_statement(compiler);
        }
        else
        {
            consume(compiler, Token_BraceL, "Expected {");
            block(compiler);
        }
    }
    patch_jump(compiler, else_jump);
}

static void loop_statement(Compiler *compiler)
{
    consume(compiler, Token_BraceL, "Expected {");

    u32 loop_start = compiler->instructions.len;
    block(compiler);
    emit_jump(compiler, Code_Goto, loop_start);
}

// we could probably remove statements and make them behave like rust does...
static void statement(Compiler *compiler)
{
    // we don't do scopes, so blocks don't do much aside reduce visual clutter
    if (match(compiler, Token_BraceL))
    {
        block(compiler);
    }
    else if (match(compiler, Token_If))
    {
        if_statement(compiler);
    }
    else if (match(compiler, Token_Loop))
    {
        loop_statement(compiler);
    }
    else
    {
        expression_statement(compiler);
    }
}

bool compiler_compile(Compiler *compiler, Event *event)
{
    if (lexer_eof(&compiler->lexer))
        return false;

    vec_init(&compiler->instructions, sizeof(Instruction));
    vec_init(&compiler->variables, sizeof(char *));

    advance(compiler);
    consume(compiler, Token_Event, "Expected event definition");
    consume(compiler, Token_String, "Expected event name");
    event->name = compiler->previous.data.string;
    consume(compiler, Token_BraceL, "Expected {");

    while (!match(compiler, Token_BraceR))
    {
        statement(compiler);
    }

    event->instructions_len = compiler->instructions.len;
    event->instructions = (Instruction *)compiler->instructions.data;

    event->slots = (char **)compiler->variables.data;
    event->slot_count = compiler->variables.len;

    return true;
}
