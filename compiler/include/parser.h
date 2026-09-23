#ifndef GHL_PARSER_H
#define GHL_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lex;
    DiagnosticEngine *diag;
    Arena *arena;
    Token current;
    bool had_error;
} Parser;

void parser_init(Parser *p, Lexer *lex, DiagnosticEngine *diag, Arena *arena);
Program *parse_program(Parser *p);

#endif
