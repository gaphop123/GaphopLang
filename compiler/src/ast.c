#include "ast.h"

Type *type_new(Arena *a, TypeKind kind) {
    Type *t = arena_alloc(a, sizeof(Type));
    t->kind = kind;
    t->base = NULL;
    t->array_size = -1;
    t->name = NULL;
    return t;
}

Type *type_ptr(Arena *a, Type *base) {
    Type *t = type_new(a, TYPE_PTR);
    t->base = base;
    return t;
}

const char *type_to_str(Type *t) {
    if (!t) return "<null>";
    switch (t->kind) {
        case TYPE_VOID: return "void";
        case TYPE_INT: return "int";
        case TYPE_UINT: return "uint";
        case TYPE_SHORT: return "short";
        case TYPE_USHORT: return "ushort";
        case TYPE_LONG: return "long";
        case TYPE_ULONG: return "ulong";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_CHAR: return "char";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_BYTE: return "byte";
        case TYPE_PTR: return "pointer";
        case TYPE_ARRAY: return "array";
        case TYPE_STRUCT: return t->name ? t->name : "struct";
        case TYPE_UNKNOWN: return "unknown";
        case TYPE_ERROR: return "error";
    }
    return "???";
}

Expr *expr_new(Arena *a, ExprKind kind, SourceLoc loc) {
    Expr *e = arena_alloc(a, sizeof(Expr));
    memset(e, 0, sizeof(Expr));
    e->kind = kind;
    e->loc = loc;
    return e;
}

Expr *expr_int(Arena *a, SourceLoc loc, int64_t v) {
    Expr *e = expr_new(a, EXPR_INT, loc);
    e->int_val = v;
    return e;
}

Expr *expr_float(Arena *a, SourceLoc loc, double v) {
    Expr *e = expr_new(a, EXPR_FLOAT, loc);
    e->float_val = v;
    return e;
}

Expr *expr_string(Arena *a, SourceLoc loc, char *s) {
    Expr *e = expr_new(a, EXPR_STRING, loc);
    e->str_val = s;
    return e;
}

Expr *expr_bool(Arena *a, SourceLoc loc, bool v) {
    Expr *e = expr_new(a, EXPR_BOOL, loc);
    e->bool_val = v;
    return e;
}

Expr *expr_char(Arena *a, SourceLoc loc, char v) {
    Expr *e = expr_new(a, EXPR_CHAR, loc);
    e->char_val = v;
    return e;
}

Expr *expr_null(Arena *a, SourceLoc loc) {
    return expr_new(a, EXPR_NULL, loc);
}

Expr *expr_ident(Arena *a, SourceLoc loc, char *name) {
    Expr *e = expr_new(a, EXPR_IDENT, loc);
    e->ident = name;
    return e;
}

Expr *expr_binary(Arena *a, SourceLoc loc, BinOp op, Expr *l, Expr *r) {
    Expr *e = expr_new(a, EXPR_BINARY, loc);
    e->binary.op = op;
    e->binary.left = l;
    e->binary.right = r;
    return e;
}

Expr *expr_unary(Arena *a, SourceLoc loc, UnOp op, Expr *operand) {
    Expr *e = expr_new(a, EXPR_UNARY, loc);
    e->unary.op = op;
    e->unary.operand = operand;
    return e;
}

Expr *expr_call(Arena *a, SourceLoc loc, Expr *callee, Expr **args, int n) {
    Expr *e = expr_new(a, EXPR_CALL, loc);
    e->call.callee = callee;
    e->call.args = args;
    e->call.arg_count = n;
    return e;
}

Expr *expr_assign(Arena *a, SourceLoc loc, Expr *target, Expr *value) {
    Expr *e = expr_new(a, EXPR_ASSIGN, loc);
    e->assign.target = target;
    e->assign.value = value;
    return e;
}

static Stmt *stmt_new(Arena *a, StmtKind kind, SourceLoc loc) {
    Stmt *s = arena_alloc(a, sizeof(Stmt));
    memset(s, 0, sizeof(Stmt));
    s->kind = kind;
    s->loc = loc;
    return s;
}

Stmt *stmt_expr(Arena *a, SourceLoc loc, Expr *e) {
    Stmt *s = stmt_new(a, STMT_EXPR, loc);
    s->expr = e;
    return s;
}

Stmt *stmt_var_decl(Arena *a, SourceLoc loc, Type *t, char *name, Expr *init, bool is_const) {
    Stmt *s = stmt_new(a, STMT_VAR_DECL, loc);
    s->var_decl.type = t;
    s->var_decl.name = name;
    s->var_decl.init = init;
    s->var_decl.is_const = is_const;
    return s;
}

Stmt *stmt_return(Arena *a, SourceLoc loc, Expr *e) {
    Stmt *s = stmt_new(a, STMT_RETURN, loc);
    s->ret_expr = e;
    return s;
}

Stmt *stmt_if(Arena *a, SourceLoc loc, Expr *cond, Stmt *then_b, Stmt *else_b) {
    Stmt *s = stmt_new(a, STMT_IF, loc);
    s->if_stmt.cond = cond;
    s->if_stmt.then_branch = then_b;
    s->if_stmt.else_branch = else_b;
    return s;
}

Stmt *stmt_while(Arena *a, SourceLoc loc, Expr *cond, Stmt *body) {
    Stmt *s = stmt_new(a, STMT_WHILE, loc);
    s->while_stmt.cond = cond;
    s->while_stmt.body = body;
    return s;
}

Stmt *stmt_block(Arena *a, SourceLoc loc, Stmt **stmts, int n) {
    Stmt *s = stmt_new(a, STMT_BLOCK, loc);
    s->block.stmts = stmts;
    s->block.count = n;
    return s;
}

Stmt *stmt_break(Arena *a, SourceLoc loc) {
    return stmt_new(a, STMT_BREAK, loc);
}

Stmt *stmt_continue(Arena *a, SourceLoc loc) {
    return stmt_new(a, STMT_CONTINUE, loc);
}

Function *func_new(Arena *a, SourceLoc loc, char *name, Type *ret,
                   Param *params, int n, Stmt *body) {
    Function *f = arena_alloc(a, sizeof(Function));
    f->name = name;
    f->ret_type = ret;
    f->params = params;
    f->param_count = n;
    f->body = body;
    f->loc = loc;
    return f;
}

Program *program_new(Arena *a) {
    Program *p = arena_alloc(a, sizeof(Program));
    p->funcs = NULL;
    p->func_count = 0;
    p->arena = a;
    return p;
}
