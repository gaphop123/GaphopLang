#include "parser.h"

static void advance(Parser *p) {
    p->current = lexer_next(p->lex);
}

static bool check(Parser *p, TokenKind k) {
    return p->current.kind == k;
}

static bool match(Parser *p, TokenKind k) {
    if (check(p, k)) {
        advance(p);
        return true;
    }
    return false;
}

static Token expect(Parser *p, TokenKind k, const char *msg) {
    if (check(p, k)) {
        Token t = p->current;
        advance(p);
        return t;
    }
    diag_error(p->diag, p->current.loc, "GHL101", "expected %s, found %s",
               msg, token_kind_str(p->current.kind));
    p->had_error = true;
    return p->current;
}

static void synchronize(Parser *p) {
    while (!check(p, TOK_EOF)) {
        if (p->current.kind == TOK_SEMI) {
            advance(p);
            return;
        }
        if (p->current.kind == TOK_FN || p->current.kind == TOK_RBRACE) return;
        advance(p);
    }
}

static Type *parse_type(Parser *p) {
    Token t = p->current;
    TypeKind kind = TYPE_UNKNOWN;
    switch (t.kind) {
        case TOK_VOID: kind = TYPE_VOID; break;
        case TOK_INT: kind = TYPE_INT; break;
        case TOK_UINT: kind = TYPE_UINT; break;
        case TOK_SHORT: kind = TYPE_SHORT; break;
        case TOK_USHORT: kind = TYPE_USHORT; break;
        case TOK_LONG: kind = TYPE_LONG; break;
        case TOK_ULONG: kind = TYPE_ULONG; break;
        case TOK_FLOAT: kind = TYPE_FLOAT; break;
        case TOK_DOUBLE: kind = TYPE_DOUBLE; break;
        case TOK_CHAR_KW: kind = TYPE_CHAR; break;
        case TOK_BOOL: kind = TYPE_BOOL; break;
        case TOK_STRING_KW: kind = TYPE_STRING; break;
        case TOK_BYTE: kind = TYPE_BYTE; break;
        default:
            diag_error(p->diag, t.loc, "GHL102", "expected type name");
            p->had_error = true;
            return type_new(p->arena, TYPE_ERROR);
    }
    advance(p);
    Type *ty = type_new(p->arena, kind);
    /* pointer: type* */
    while (match(p, TOK_STAR)) {
        ty = type_ptr(p->arena, ty);
    }
    return ty;
}

/* Forward decls */
static Expr *parse_expr(Parser *p);
static Stmt *parse_stmt(Parser *p);
static Stmt *parse_block(Parser *p);

static Expr *parse_primary(Parser *p) {
    Token t = p->current;
    if (match(p, TOK_INT_LIT)) {
        return expr_int(p->arena, t.loc, t.int_val);
    }
    if (match(p, TOK_FLOAT_LIT)) {
        return expr_float(p->arena, t.loc, t.float_val);
    }
    if (match(p, TOK_STRING)) {
        return expr_string(p->arena, t.loc, t.str_val);
    }
    if (match(p, TOK_CHAR)) {
        return expr_char(p->arena, t.loc, t.char_val);
    }
    if (match(p, TOK_TRUE)) return expr_bool(p->arena, t.loc, true);
    if (match(p, TOK_FALSE)) return expr_bool(p->arena, t.loc, false);
    if (match(p, TOK_NULL)) return expr_null(p->arena, t.loc);
    if (match(p, TOK_IDENT)) {
        return expr_ident(p->arena, t.loc, (char *)t.lexeme);
    }
    if (match(p, TOK_LPAREN)) {
        Expr *e = parse_expr(p);
        expect(p, TOK_RPAREN, "')'");
        return e;
    }
    diag_error(p->diag, t.loc, "GHL103", "expected expression");
    p->had_error = true;
    advance(p);
    return expr_int(p->arena, t.loc, 0);
}

static Expr *parse_postfix(Parser *p) {
    Expr *e = parse_primary(p);
    for (;;) {
        if (match(p, TOK_LPAREN)) {
            /* call */
            Expr **args = NULL;
            int count = 0;
            int cap = 0;
            if (!check(p, TOK_RPAREN)) {
                do {
                    if (count >= cap) {
                        cap = cap ? cap * 2 : 4;
                        Expr **na = arena_alloc(p->arena, sizeof(Expr *) * cap);
                        if (args) memcpy(na, args, sizeof(Expr *) * count);
                        args = na;
                    }
                    args[count++] = parse_expr(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, "')'");
            e = expr_call(p->arena, e->loc, e, args, count);
        } else if (match(p, TOK_INC)) {
            e = expr_unary(p->arena, e->loc, UN_POST_INC, e);
        } else if (match(p, TOK_DEC)) {
            e = expr_unary(p->arena, e->loc, UN_POST_DEC, e);
        } else {
            break;
        }
    }
    return e;
}

static Expr *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_NEG, op);
    }
    if (match(p, TOK_NOT)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_NOT, op);
    }
    if (match(p, TOK_TILDE)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_BIT_NOT, op);
    }
    if (match(p, TOK_AMP)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_ADDR, op);
    }
    if (match(p, TOK_STAR)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_DEREF, op);
    }
    if (match(p, TOK_INC)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_PRE_INC, op);
    }
    if (match(p, TOK_DEC)) {
        Expr *op = parse_unary(p);
        return expr_unary(p->arena, op->loc, UN_PRE_DEC, op);
    }
    return parse_postfix(p);
}

static Expr *parse_mul(Parser *p) {
    Expr *e = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        Token op = p->current;
        advance(p);
        Expr *r = parse_unary(p);
        BinOp b = op.kind == TOK_STAR ? BIN_MUL : (op.kind == TOK_SLASH ? BIN_DIV : BIN_MOD);
        e = expr_binary(p->arena, op.loc, b, e, r);
    }
    return e;
}

static Expr *parse_add(Parser *p) {
    Expr *e = parse_mul(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token op = p->current;
        advance(p);
        Expr *r = parse_mul(p);
        e = expr_binary(p->arena, op.loc, op.kind == TOK_PLUS ? BIN_ADD : BIN_SUB, e, r);
    }
    return e;
}

static Expr *parse_cmp(Parser *p) {
    Expr *e = parse_add(p);
    while (check(p, TOK_LT) || check(p, TOK_GT) || check(p, TOK_LE) || check(p, TOK_GE) ||
           check(p, TOK_EQ) || check(p, TOK_NE)) {
        Token op = p->current;
        advance(p);
        Expr *r = parse_add(p);
        BinOp b;
        switch (op.kind) {
            case TOK_LT: b = BIN_LT; break;
            case TOK_GT: b = BIN_GT; break;
            case TOK_LE: b = BIN_LE; break;
            case TOK_GE: b = BIN_GE; break;
            case TOK_EQ: b = BIN_EQ; break;
            default: b = BIN_NE; break;
        }
        e = expr_binary(p->arena, op.loc, b, e, r);
    }
    return e;
}

static Expr *parse_and(Parser *p) {
    Expr *e = parse_cmp(p);
    while (match(p, TOK_AND)) {
        Expr *r = parse_cmp(p);
        e = expr_binary(p->arena, e->loc, BIN_AND, e, r);
    }
    return e;
}

static Expr *parse_or(Parser *p) {
    Expr *e = parse_and(p);
    while (match(p, TOK_OR)) {
        Expr *r = parse_and(p);
        e = expr_binary(p->arena, e->loc, BIN_OR, e, r);
    }
    return e;
}

static Expr *parse_assign(Parser *p) {
    Expr *e = parse_or(p);
    if (match(p, TOK_ASSIGN)) {
        Expr *val = parse_assign(p);
        return expr_assign(p->arena, e->loc, e, val);
    }
    /* compound assign */
    TokenKind compounds[] = {TOK_PLUS_EQ, TOK_MINUS_EQ, TOK_STAR_EQ, TOK_SLASH_EQ, TOK_PERCENT_EQ};
    BinOp ops[] = {BIN_ADD, BIN_SUB, BIN_MUL, BIN_DIV, BIN_MOD};
    for (int i = 0; i < 5; i++) {
        if (match(p, compounds[i])) {
            Expr *val = parse_assign(p);
            Expr *bin = expr_binary(p->arena, e->loc, ops[i], e, val);
            return expr_assign(p->arena, e->loc, e, bin);
        }
    }
    return e;
}

static Expr *parse_expr(Parser *p) {
    return parse_assign(p);
}

static Stmt *parse_var_decl(Parser *p, bool is_const) {
    SourceLoc loc = p->current.loc;
    Type *ty = parse_type(p);
    Token name = expect(p, TOK_IDENT, "identifier");
    Expr *init = NULL;
    if (match(p, TOK_ASSIGN)) {
        init = parse_expr(p);
    }
    expect(p, TOK_SEMI, "';'");
    return stmt_var_decl(p->arena, loc, ty, (char *)name.lexeme, init, is_const);
}

static Stmt *parse_if(Parser *p) {
    SourceLoc loc = p->current.loc;
    expect(p, TOK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect(p, TOK_RPAREN, "')'");
    Stmt *then_b = parse_stmt(p);
    Stmt *else_b = NULL;
    if (match(p, TOK_ELSE)) {
        else_b = parse_stmt(p);
    }
    return stmt_if(p->arena, loc, cond, then_b, else_b);
}

static Stmt *parse_while(Parser *p) {
    SourceLoc loc = p->current.loc;
    expect(p, TOK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect(p, TOK_RPAREN, "')'");
    Stmt *body = parse_stmt(p);
    return stmt_while(p->arena, loc, cond, body);
}

static Stmt *parse_return(Parser *p) {
    SourceLoc loc = p->current.loc;
    Expr *e = NULL;
    if (!check(p, TOK_SEMI)) {
        e = parse_expr(p);
    }
    expect(p, TOK_SEMI, "';'");
    return stmt_return(p->arena, loc, e);
}

static Stmt *parse_block(Parser *p) {
    SourceLoc loc = p->current.loc;
    expect(p, TOK_LBRACE, "'{'");
    Stmt **stmts = NULL;
    int count = 0, cap = 0;
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        if (count >= cap) {
            cap = cap ? cap * 2 : 8;
            Stmt **ns = arena_alloc(p->arena, sizeof(Stmt *) * cap);
            if (stmts) memcpy(ns, stmts, sizeof(Stmt *) * count);
            stmts = ns;
        }
        stmts[count++] = parse_stmt(p);
    }
    expect(p, TOK_RBRACE, "'}'");
    return stmt_block(p->arena, loc, stmts, count);
}

static Stmt *parse_stmt(Parser *p) {
    if (check(p, TOK_LBRACE)) return parse_block(p);
    if (match(p, TOK_IF)) return parse_if(p);
    if (match(p, TOK_WHILE)) return parse_while(p);
    if (match(p, TOK_RETURN)) return parse_return(p);
    if (match(p, TOK_BREAK)) {
        SourceLoc loc = p->current.loc;
        expect(p, TOK_SEMI, "';'");
        return stmt_break(p->arena, loc);
    }
    if (match(p, TOK_CONTINUE)) {
        SourceLoc loc = p->current.loc;
        expect(p, TOK_SEMI, "';'");
        return stmt_continue(p->arena, loc);
    }
    if (match(p, TOK_CONST)) return parse_var_decl(p, true);
    /* type keyword starts var decl */
    if (p->current.kind == TOK_INT || p->current.kind == TOK_UINT ||
        p->current.kind == TOK_FLOAT || p->current.kind == TOK_DOUBLE ||
        p->current.kind == TOK_BOOL || p->current.kind == TOK_STRING_KW ||
        p->current.kind == TOK_CHAR_KW || p->current.kind == TOK_BYTE ||
        p->current.kind == TOK_SHORT || p->current.kind == TOK_LONG ||
        p->current.kind == TOK_USHORT || p->current.kind == TOK_ULONG) {
        return parse_var_decl(p, false);
    }
    /* expression statement */
    Expr *e = parse_expr(p);
    expect(p, TOK_SEMI, "';'");
    return stmt_expr(p->arena, e->loc, e);
}

static Function *parse_function(Parser *p) {
    SourceLoc loc = p->current.loc;
    expect(p, TOK_FN, "'fn'");
    Token name = expect(p, TOK_IDENT, "function name");
    expect(p, TOK_LPAREN, "'('");

    Param *params = NULL;
    int pcount = 0, pcap = 0;
    if (!check(p, TOK_RPAREN)) {
        do {
            if (pcount >= pcap) {
                pcap = pcap ? pcap * 2 : 4;
                Param *np = arena_alloc(p->arena, sizeof(Param) * pcap);
                if (params) memcpy(np, params, sizeof(Param) * pcount);
                params = np;
            }
            Type *ty = parse_type(p);
            Token pn = expect(p, TOK_IDENT, "parameter name");
            params[pcount].type = ty;
            params[pcount].name = (char *)pn.lexeme;
            params[pcount].loc = pn.loc;
            pcount++;
        } while (match(p, TOK_COMMA));
    }
    expect(p, TOK_RPAREN, "')'");
    expect(p, TOK_ARROW, "'->'");
    Type *ret = parse_type(p);
    Stmt *body = parse_block(p);
    return func_new(p->arena, loc, (char *)name.lexeme, ret, params, pcount, body);
}

void parser_init(Parser *p, Lexer *lex, DiagnosticEngine *diag, Arena *arena) {
    p->lex = lex;
    p->diag = diag;
    p->arena = arena;
    p->had_error = false;
    advance(p);
}

Program *parse_program(Parser *p) {
    Program *prog = program_new(p->arena);
    int cap = 0;
    while (!check(p, TOK_EOF)) {
        if (check(p, TOK_FN)) {
            if (prog->func_count >= cap) {
                cap = cap ? cap * 2 : 4;
                Function **nf = arena_alloc(p->arena, sizeof(Function *) * cap);
                if (prog->funcs) memcpy(nf, prog->funcs, sizeof(Function *) * prog->func_count);
                prog->funcs = nf;
            }
            prog->funcs[prog->func_count++] = parse_function(p);
        } else {
            diag_error(p->diag, p->current.loc, "GHL104", "expected function declaration");
            p->had_error = true;
            synchronize(p);
        }
    }
    return prog;
}
