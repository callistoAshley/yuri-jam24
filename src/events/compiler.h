#pragma once

// our "language" is so simple that we use a single-pass compiler- most
// languages would have an intermediate parser that spits out an AST, but we
// need no such thing! we can just emit instructions directly

#include "events/instruction.h"
#include "events/lexer.h"
#include "utility/vec.h"
typedef struct
{
    Lexer lexer;

    Token current;
    Token previous;

    vec instructions; // vec<Instruction>
} Compiler;

typedef struct
{
    const char *name;

    Instruction *instructions;
    u32 instructions_len;

    // number of required slots
    u32 required_slots;
} Event;

void compiler_init(Compiler *compiler, const char *source);
// compiles the next event.
// returns true until there are no events left.
// will exit the program if it fails to compile
// (we do not bother handling errors gracefully lol)
// instructions can be retrieved from the instructions field.
bool compiler_compile(Compiler *compiler, Event *event);
