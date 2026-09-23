#include "lexer.h"

static const struct { const char *kw; TokenKind kind; } keywords[] = {
    {"fn", TOK_FN}, {"let", TOK_LET}, {"const", TOK_CONST},
    {"if", TOK_IF}, {"else", TOK_ELSE}, {"while", TOK_WHILE},
    {"for", TOK_FOR}, {"return", TOK_RETURN}, {"break", TOK_BREAK},
    {"continue", TOK_CONTINUE}, {"struct", TOK_STRUCT}, {"enum", TOK_ENUM},
    {"import", TOK_IMPORT}, {"switch", TOK_SWITCH}, {"case", TOK_CASE},
    {"default", TOK_DEFAULT}, {"true", TOK_TRUE}, {"false", TOK_FALSE},
    {"null", TOK_NULL}, {"void", TOK_VOID},
    {"int", TOK_INT}, {"uint", TOK_UINT}, {"short", TOK_SHORT},
    {"ushort", TOK_USHORT}, {"long", TOK_LONG}, {"ulong", TOK_ULONG},
    {"float", TOK_FLOAT}, {"double", TOK_DOUBLE}, {"char", TOK_CHAR_KW},
    {"bool", TOK_BOOL}, {"string", TOK_STRING_KW}, {"byte", TOK_BYTE},
    {NULL, 0}
};

const char *token_kind_str(TokenKind k) {
    switch (k) {
        case TOK_EOF: return "EOF";
        case TOK_IDENT: return "identifier";
        case TOK_INT_LIT: return "integer";
        case TOK_FLOAT_LIT: return "float";
        case TOK_STRING: return "string";
        case TOK_CHAR: return "char";
        case TOK_FN: return "fn";
        case TOK_RETURN: return "return";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_NULL: return "null";
        case TOK_VOID: return "void";
        case TOK_INT: return "int";
        case TOK_FLOAT: return "float";
        case TOK_BOOL: return "bool";
        case TOK_STRING_KW: return "string";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_ASSIGN: return "=";
        case TOK_EQ: return "==";
        case TOK_NE: return "!=";
        case TOK_LT: return "<";
        case TOK_GT: return ">";
        case TOK_LE: return "<=";
        case TOK_GE: return ">=";
        case TOK_AND: return "&&";
        case TOK_OR: return "||";
        case TOK_NOT: return "!";
        case TOK_ARROW: return "->";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_SEMI: return ";";
        case TOK_COMMA: return ",";
        case TOK_COLON: return ":";
        default: return "token";
    }
}

void lexer_init(Lexer *lex, const char *src, size_t len,
                const char *filename, DiagnosticEngine *diag, Arena *arena) {
    lex->src = src;
    lex->len = len;
    lex->pos = 0;
    lex->line = 1;
    lex->column = 1;
    lex->filename = filename;
    lex->diag = diag;
    lex->arena = arena;
    lex->has_lookahead = false;
}

static char peek_char(Lexer *lex) {
    if (lex->pos >= lex->len) return '\0';
    return lex->src[lex->pos];
}

static char peek_next(Lexer *lex) {
    if (lex->pos + 1 >= lex->len) return '\0';
    return lex->src[lex->pos + 1];
}

static char advance(Lexer *lex) {
    if (lex->pos >= lex->len) return '\0';
    char c = lex->src[lex->pos++];
    if (c == '\n') {
        lex->line++;
        lex->column = 1;
    } else {
        lex->column++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lex) {
    for (;;) {
        char c = peek_char(lex);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lex);
            continue;
        }
        if (c == '/' && peek_next(lex) == '/') {
            /* line comment */
            while (peek_char(lex) && peek_char(lex) != '\n') advance(lex);
            continue;
        }
        if (c == '/' && peek_next(lex) == '*') {
            advance(lex); advance(lex);
            while (peek_char(lex)) {
                if (peek_char(lex) == '*' && peek_next(lex) == '/') {
                    advance(lex); advance(lex);
                    break;
                }
                advance(lex);
            }
            continue;
        }
        break;
    }
}

static Token make_token(Lexer *lex, TokenKind kind, int start_col) {
    Token t = {0};
    t.kind = kind;
    t.loc.filename = lex->filename;
    t.loc.line = lex->line;
    t.loc.column = start_col;
    return t;
}

static Token lex_number(Lexer *lex, int start_col) {
    size_t start = lex->pos;
    bool is_float = false;
    while (isdigit(peek_char(lex))) advance(lex);
    if (peek_char(lex) == '.' && isdigit(peek_next(lex))) {
        is_float = true;
        advance(lex);
        while (isdigit(peek_char(lex))) advance(lex);
    }
    /* simple exponent */
    if (peek_char(lex) == 'e' || peek_char(lex) == 'E') {
        is_float = true;
        advance(lex);
        if (peek_char(lex) == '+' || peek_char(lex) == '-') advance(lex);
        while (isdigit(peek_char(lex))) advance(lex);
    }

    size_t len = lex->pos - start;
    char *buf = arena_alloc(lex->arena, len + 1);
    memcpy(buf, lex->src + start, len);
    buf[len] = '\0';

    Token t = make_token(lex, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, start_col);
    t.lexeme = buf;
    t.lexeme_len = (int)len;
    if (is_float) t.float_val = atof(buf);
    else t.int_val = atoll(buf);
    return t;
}

static Token lex_string(Lexer *lex, int start_col) {
    advance(lex); /* skip " */
    StringBuilder sb;
    sb_init(&sb);
    while (peek_char(lex) && peek_char(lex) != '"') {
        if (peek_char(lex) == '\\') {
            advance(lex);
            char e = advance(lex);
            switch (e) {
                case 'n': sb_append(&sb, "\n"); break;
                case 't': sb_append(&sb, "\t"); break;
                case 'r': sb_append(&sb, "\r"); break;
                case '\\': sb_append(&sb, "\\"); break;
                case '"': sb_append(&sb, "\""); break;
                case '0': sb_append(&sb, "\0"); break;
                default: {
                    char tmp[2] = {e, 0};
                    sb_append(&sb, tmp);
                    break;
                }
            }
        } else {
            char c = advance(lex);
            char tmp[2] = {c, 0};
            sb_append(&sb, tmp);
        }
    }
    if (peek_char(lex) != '"') {
        SourceLoc loc = {lex->filename, lex->line, start_col};
        diag_error(lex->diag, loc, "GHL001", "unterminated string literal");
        Token t = make_token(lex, TOK_ERROR, start_col);
        sb_free(&sb);
        return t;
    }
    advance(lex); /* closing " */

    Token t = make_token(lex, TOK_STRING, start_col);
    t.str_val = sb_to_string(&sb);
    t.lexeme = t.str_val;
    t.lexeme_len = (int)strlen(t.str_val);
    return t;
}

static Token lex_char(Lexer *lex, int start_col) {
    advance(lex); /* ' */
    char val = 0;
    if (peek_char(lex) == '\\') {
        advance(lex);
        char e = advance(lex);
        switch (e) {
            case 'n': val = '\n'; break;
            case 't': val = '\t'; break;
            case 'r': val = '\r'; break;
            case '\\': val = '\\'; break;
            case '\'': val = '\''; break;
            case '0': val = '\0'; break;
            default: val = e; break;
        }
    } else {
        val = advance(lex);
    }
    if (peek_char(lex) != '\'') {
        SourceLoc loc = {lex->filename, lex->line, start_col};
        diag_error(lex->diag, loc, "GHL002", "unterminated character literal");
        return make_token(lex, TOK_ERROR, start_col);
    }
    advance(lex);
    Token t = make_token(lex, TOK_CHAR, start_col);
    t.char_val = val;
    return t;
}

static Token lex_ident_or_kw(Lexer *lex, int start_col) {
    size_t start = lex->pos;
    while (isalnum(peek_char(lex)) || peek_char(lex) == '_') advance(lex);
    size_t len = lex->pos - start;
    char *buf = arena_alloc(lex->arena, len + 1);
    memcpy(buf, lex->src + start, len);
    buf[len] = '\0';

    TokenKind kind = TOK_IDENT;
    for (int i = 0; keywords[i].kw; i++) {
        if (strcmp(buf, keywords[i].kw) == 0) {
            kind = keywords[i].kind;
            break;
        }
    }
    Token t = make_token(lex, kind, start_col);
    t.lexeme = buf;
    t.lexeme_len = (int)len;
    return t;
}

Token lexer_next(Lexer *lex) {
    if (lex->has_lookahead) {
        lex->has_lookahead = false;
        return lex->lookahead;
    }

    skip_whitespace_and_comments(lex);
    int start_col = lex->column;
    char c = peek_char(lex);

    if (c == '\0') return make_token(lex, TOK_EOF, start_col);

    if (isalpha(c) || c == '_') return lex_ident_or_kw(lex, start_col);
    if (isdigit(c)) return lex_number(lex, start_col);
    if (c == '"') return lex_string(lex, start_col);
    if (c == '\'') return lex_char(lex, start_col);

    /* multi-char operators */
    if (c == '+' && peek_next(lex) == '+') { advance(lex); advance(lex); return make_token(lex, TOK_INC, start_col); }
    if (c == '-' && peek_next(lex) == '-') { advance(lex); advance(lex); return make_token(lex, TOK_DEC, start_col); }
    if (c == '-' && peek_next(lex) == '>') { advance(lex); advance(lex); return make_token(lex, TOK_ARROW, start_col); }
    if (c == '=' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_EQ, start_col); }
    if (c == '!' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_NE, start_col); }
    if (c == '<' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_LE, start_col); }
    if (c == '>' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_GE, start_col); }
    if (c == '&' && peek_next(lex) == '&') { advance(lex); advance(lex); return make_token(lex, TOK_AND, start_col); }
    if (c == '|' && peek_next(lex) == '|') { advance(lex); advance(lex); return make_token(lex, TOK_OR, start_col); }
    if (c == '<' && peek_next(lex) == '<') { advance(lex); advance(lex); return make_token(lex, TOK_SHL, start_col); }
    if (c == '>' && peek_next(lex) == '>') { advance(lex); advance(lex); return make_token(lex, TOK_SHR, start_col); }
    if (c == '+' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_PLUS_EQ, start_col); }
    if (c == '-' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_MINUS_EQ, start_col); }
    if (c == '*' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_STAR_EQ, start_col); }
    if (c == '/' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_SLASH_EQ, start_col); }
    if (c == '%' && peek_next(lex) == '=') { advance(lex); advance(lex); return make_token(lex, TOK_PERCENT_EQ, start_col); }

    advance(lex);
    switch (c) {
        case '+': return make_token(lex, TOK_PLUS, start_col);
        case '-': return make_token(lex, TOK_MINUS, start_col);
        case '*': return make_token(lex, TOK_STAR, start_col);
        case '/': return make_token(lex, TOK_SLASH, start_col);
        case '%': return make_token(lex, TOK_PERCENT, start_col);
        case '=': return make_token(lex, TOK_ASSIGN, start_col);
        case '<': return make_token(lex, TOK_LT, start_col);
        case '>': return make_token(lex, TOK_GT, start_col);
        case '!': return make_token(lex, TOK_NOT, start_col);
        case '&': return make_token(lex, TOK_AMP, start_col);
        case '|': return make_token(lex, TOK_PIPE, start_col);
        case '^': return make_token(lex, TOK_CARET, start_col);
        case '~': return make_token(lex, TOK_TILDE, start_col);
        case '(': return make_token(lex, TOK_LPAREN, start_col);
        case ')': return make_token(lex, TOK_RPAREN, start_col);
        case '{': return make_token(lex, TOK_LBRACE, start_col);
        case '}': return make_token(lex, TOK_RBRACE, start_col);
        case '[': return make_token(lex, TOK_LBRACKET, start_col);
        case ']': return make_token(lex, TOK_RBRACKET, start_col);
        case ';': return make_token(lex, TOK_SEMI, start_col);
        case ',': return make_token(lex, TOK_COMMA, start_col);
        case '.': return make_token(lex, TOK_DOT, start_col);
        case ':': return make_token(lex, TOK_COLON, start_col);
        default: {
            SourceLoc loc = {lex->filename, lex->line, start_col};
            diag_error(lex->diag, loc, "GHL003", "unexpected character '%c'", c);
            return make_token(lex, TOK_ERROR, start_col);
        }
    }
}

Token lexer_peek(Lexer *lex) {
    if (!lex->has_lookahead) {
        lex->lookahead = lexer_next(lex);
        lex->has_lookahead = true;
    }
    return lex->lookahead;
}
