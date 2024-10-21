#pragma once

// our "language" is so simple that we use a single-pass compiler- most
// languages would have an intermediate parser that spits out an AST, but we
// need no such thing! we can just emit instructions directly

#include "events/event.h"
#include "events/lexer.h"
#include "utility/vec.h"

typedef struct
{
    Lexer lexer;
    bool is_primed;

    Token current;
    Token previous;

    vec instructions; // vec<Instruction>

    vec unresolved_gotos; // vec<(char*, u32)> (indexes into instructions)
    vec labels; // vec<(char*, u32)> (label name and instruction index)

    // list of variables. whenever the compiler finds a mention of a variable,
    // it adds it to this list. the compiler never emits variable names though-
    // only "slots" that variables are stored in.
    // the interpreter is expected to reserve these slots for variables
    vec variables; // vec<char*>
} Compiler;

void compiler_init(Compiler *compiler, const char *source);
// compiles the next event.
// returns true until there are no events left.
// will exit the program if it fails to compile
// (we do not bother handling errors gracefully lol)
// instructions can be retrieved from the instructions field.
bool compiler_compile(Compiler *compiler, Event *event);
