#include "compiler.h"
#include "events/lexer.h"
#include "events/instruction.h"
#include "utility/macros.h"
#include "utility/vec.h"
#include <string.h>

void compiler_init(Compiler *compiler, const char *source)
{
    lexer_init(&compiler->lexer, source);
    compiler->is_primed = false;
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

// used for back-patching gotos.
typedef struct
{
    // either the name of the label, or the label a goto is expecting
    char *label;
    // the instruction the goto/label is at
    u32 instruction;
} LabelDef;

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
    Instruction instruction = {.code = code, .data.position = UINT32_MAX};
    emit(compiler, instruction);
    return current_instruction;
}

static void patch_jump(Compiler *compiler, u32 offset)
{
    u32 jump_pos = compiler->instructions.len;

    Instruction *instruction = vec_get(&compiler->instructions, offset);
    instruction->data.position = jump_pos;
}

static void emit_jump(Compiler *compiler, InstructionCode code, u32 position)
{
    Instruction instruction = {.code = code, .data.position = position};
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

static u32 argument_list(Compiler *compiler)
{

    u32 count = 0;
    if (!check(compiler, Token_ParenR))
    {
        do
        {
            expression(compiler);
            count++;
        } while (match(compiler, Token_Comma));
    }
    consume(compiler, Token_ParenR, "Expected ')' after argument list");
    return count;
}

// assumes the initial ( was consumed
// we don't support calling from anything but a hardcoded identifier so that has
// to be handled in variable()
static void grouping(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    expression(compiler);
    consume(compiler, Token_ParenR, "Expected '')' after grouping");
}

static void call(Compiler *compiler, char *command)
{
    u32 arg_count = argument_list(compiler);
    Instruction instruction = {
        .code = Code_Call,
        .data.call = {command, arg_count},
    };
    emit(compiler, instruction);
}

// will free the variable name if it is already present!
static u32 get_or_insert_variable(Compiler *compiler, char *name)
{
    for (u32 i = 0; i < compiler->variables.len; i++)
    {
        char *var = *(char **)vec_get(&compiler->variables, i);
        if (!strcmp(name, var))
        {
            free(name);
            return i;
        }
    }

    // looks like this variable hasn't been used yet.
    // push it to the array and return the slot
    u32 slot = compiler->variables.len;
    vec_push(&compiler->variables, &name);
    return slot;
}

static void identifier(Compiler *compiler, bool can_assign)
{
    // turns out this was a command call. handle that and return immediately
    char *ident = compiler->previous.data.ident;
    if (match(compiler, Token_ParenL))
    {
        call(compiler, ident);
        return;
    }

    u32 slot = get_or_insert_variable(compiler, ident);
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

// we don't actually have instructions for boolean logic. instead we
// short-circuit
//
// (left) & (right)
//
// we implement & by jumping over the right
// side if the left side was false
static void and_operator(Compiler *compiler, bool can_assign)
{
    (void)can_assign;
    u32 end_jump = emit_unknown_jump(compiler, Code_GotoIfFalse);

    emit_basic(compiler, Code_Pop);
    parse_precedence(compiler, Prec_And);

    patch_jump(compiler, end_jump);
}

// we don't actually have instructions for boolean logic. instead we
// short-circuit
//
// (left) | (right)
//
// we implement | by jumping over the right
// side if the left side was true
static void or_operator(Compiler *compiler, bool can_assign)
{

    (void)can_assign;
    u32 end_jump = emit_unknown_jump(compiler, Code_GotoIfTrue);

    emit_basic(compiler, Code_Pop);
    parse_precedence(compiler, Prec_Or);

    patch_jump(compiler, end_jump);
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
    [Token_Ident] = {identifier, NULL, Prec_None},
    [Token_Label] = NULL_RULE,

    // braces
    [Token_BraceL] = NULL_RULE,
    [Token_BraceR] = NULL_RULE,

    // grouping
    // we don't support calling from anything but a hardcoded identifier, so we
    // don't perform call parsing here
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
    [Token_And] = {NULL, and_operator, Prec_And},
    [Token_Or] = {NULL, or_operator, Prec_Or},

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

    consume(compiler, Token_BraceR, "Expected '}' after block");
}

static void expression_statement(Compiler *compiler)
{
    expression(compiler);
    consume(compiler, Token_Semicolon, "Expected ';' after expression");
    emit_basic(compiler, Code_Pop);
}

static void if_statement(Compiler *compiler)
{
    // we don't do the () around ifs, and ifs always have to have a block
    expression(compiler);
    consume(compiler, Token_BraceL, "Expected '{' after if");

    // skip over if branch if condition was false
    u32 if_jump = emit_unknown_jump(compiler, Code_GotoIfFalse);
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
            consume(compiler, Token_BraceL, "Expected '{' after else");
            block(compiler);
        }
    }
    patch_jump(compiler, else_jump);
}

static void loop_statement(Compiler *compiler)
{
    consume(compiler, Token_BraceL, "Expected '{' after loop");

    u32 loop_start = compiler->instructions.len;
    block(compiler);
    emit_jump(compiler, Code_Goto, loop_start);
}

// tries to find a label.
// returns false if no label could be found, or true if one was found.
// fills out the location if a label was found.
static bool find_label(Compiler *compiler, char *wanted, u32 *location)
{
    for (u32 i = 0; i < compiler->labels.len; i++)
    {
        LabelDef *label = vec_get(&compiler->labels, i);
        if (!strcmp(label->label, wanted))
        {
            *location = i;
            return true;
        }
    }

    return false;
}

static void goto_statement(Compiler *compiler)
{
    consume(compiler, Token_Ident, "Expected label after goto");
    char *label = compiler->previous.data.ident;
    consume(compiler, Token_Semicolon, "Expected ';' after goto label");

    u32 jump_position;
    if (find_label(compiler, label, &jump_position))
    {
        free(label);
        // looks like the label was defined before the goto, and we can fill it
        // in!
        emit_jump(compiler, Code_Goto, jump_position);
    }
    else
    {
        // we'll need to find it later...
        u32 goto_position = emit_unknown_jump(compiler, Code_Goto);
        LabelDef to_backfill = {.label = label, .instruction = goto_position};
        vec_push(&compiler->unresolved_gotos, &to_backfill);
    }
}

static void label_statement(Compiler *compiler)
{
    u32 label_position = compiler->instructions.len;
    char *label = compiler->previous.data.label;
    LabelDef definition = {.label = label, .instruction = label_position};
    vec_push(&compiler->labels, &definition);
}

static void free_label_def(usize i, void *arg)
{
    (void)i;
    LabelDef *def = arg;
    free(def->label);
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
    else if (match(compiler, Token_Goto))
    {
        goto_statement(compiler);
    }
    else if (match(compiler, Token_Label))
    {
        label_statement(compiler);
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

    vec_init(&compiler->labels, sizeof(LabelDef));
    vec_init(&compiler->unresolved_gotos, sizeof(LabelDef));

    // when the compiler is first initialized, current and previous are
    // uninitialized and need to be primed
    if (!compiler->is_primed)
    {
        advance(compiler);
        compiler->is_primed = true;
    }
    consume(compiler, Token_Event, "Expected event definition");
    consume(compiler, Token_String, "Expected event name");
    event->name = compiler->previous.data.string;
    consume(compiler, Token_BraceL, "Expected block after event name");

    block(compiler);

    // resolve any unresolved gotos
    for (u32 i = 0; i < compiler->unresolved_gotos.len; i++)
    {
        LabelDef *to_backfill = vec_get(&compiler->unresolved_gotos, i);

        u32 label_position;
        if (!find_label(compiler, to_backfill->label, &label_position))
        {
            FATAL("Undefined label %s", to_backfill->label);
        }

        Instruction *instruction =
            vec_get(&compiler->instructions, to_backfill->instruction);
        instruction->data.position = label_position;
        free(to_backfill->label);
    }
    vec_free(&compiler->unresolved_gotos);

    vec_free_with(&compiler->labels, free_label_def);

    event->instructions_len = compiler->instructions.len;
    event->instructions = (Instruction *)compiler->instructions.data;

    event->slots = (char **)compiler->variables.data;
    event->slot_count = compiler->variables.len;

    return true;
}
