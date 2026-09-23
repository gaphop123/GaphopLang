#ifndef GHL_LEXER_H
#define GHL_LEXER_H

#include "common.h"
#include "diagnostics.h"

typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING,
    TOK_CHAR,

    /* Keywords */
    TOK_FN,
    TOK_LET,
    TOK_CONST,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_STRUCT,
    TOK_ENUM,
    TOK_IMPORT,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_VOID,
    TOK_INT,
    TOK_UINT,
    TOK_SHORT,
    TOK_USHORT,
    TOK_LONG,
    TOK_ULONG,
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_CHAR_KW,
    TOK_BOOL,
    TOK_STRING_KW,
    TOK_BYTE,

    /* Operators & punctuation */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_PERCENT,    /* % */
    TOK_EQ,         /* == */
    TOK_NE,         /* != */
    TOK_LT,         /* < */
    TOK_GT,         /* > */
    TOK_LE,         /* <= */
    TOK_GE,         /* >= */
    TOK_AND,        /* && */
    TOK_OR,         /* || */
    TOK_NOT,        /* ! */
    TOK_AMP,        /* & */
    TOK_PIPE,       /* | */
    TOK_CARET,      /* ^ */
    TOK_TILDE,      /* ~ */
    TOK_SHL,        /* << */
    TOK_SHR,        /* >> */
    TOK_ASSIGN,     /* = */
    TOK_PLUS_EQ,    /* += */
    TOK_MINUS_EQ,   /* -= */
    TOK_STAR_EQ,    /* *= */
    TOK_SLASH_EQ,   /* /= */
    TOK_PERCENT_EQ, /* %= */
    TOK_INC,        /* ++ */
    TOK_DEC,        /* -- */
    TOK_ARROW,      /* -> */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_LBRACKET,   /* [ */
    TOK_RBRACKET,   /* ] */
    TOK_SEMI,       /* ; */
    TOK_COMMA,      /* , */
    TOK_DOT,        /* . */
    TOK_COLON,      /* : */

    TOK_ERROR
} TokenKind;

typedef struct {
    TokenKind kind;
    SourceLoc loc;
    union {
        int64_t int_val;
        double  float_val;
        char   *str_val;   /* owned by arena or strdup for strings */
        char    char_val;
    };
    const char *lexeme;    /* pointer into source or arena */
    int lexeme_len;
} Token;

typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    int line;
    int column;
    const char *filename;
    DiagnosticEngine *diag;
    Arena *arena;
    Token current;
    Token lookahead;
    bool has_lookahead;
} Lexer;

void lexer_init(Lexer *lex, const char *src, size_t len,
                const char *filename, DiagnosticEngine *diag, Arena *arena);
Token lexer_next(Lexer *lex);
Token lexer_peek(Lexer *lex);
const char *token_kind_str(TokenKind k);

#endif
