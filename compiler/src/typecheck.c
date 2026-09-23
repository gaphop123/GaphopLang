#include "typecheck.h"
#include <stdio.h>

typedef struct Symbol {
    char *name;
    Type *type;
    bool is_const;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
} Scope;

typedef struct {
    DiagnosticEngine *diag;
    Arena *arena;
    Scope *scope;
    Function *current_func;
    /* builtin functions */
    Symbol *builtins;
} TypeChecker;

static Type *type_int(TypeChecker *tc) { return type_new(tc->arena, TYPE_INT); }
static Type *type_bool(TypeChecker *tc) { return type_new(tc->arena, TYPE_BOOL); }
static Type *type_string(TypeChecker *tc) { return type_new(tc->arena, TYPE_STRING); }
static Type *type_void(TypeChecker *tc) { return type_new(tc->arena, TYPE_VOID); }
static Type *type_error(TypeChecker *tc) { return type_new(tc->arena, TYPE_ERROR); }

static bool type_eq(Type *a, Type *b) {
    if (!a || !b) return false;
    if (a->kind == TYPE_ERROR || b->kind == TYPE_ERROR) return true; /* recover */
    if (a->kind != b->kind) return false;
    if (a->kind == TYPE_PTR) return type_eq(a->base, b->base);
    return true;
}

static void push_scope(TypeChecker *tc) {
    Scope *s = arena_alloc(tc->arena, sizeof(Scope));
    s->symbols = NULL;
    s->parent = tc->scope;
    tc->scope = s;
}

static void pop_scope(TypeChecker *tc) {
    if (tc->scope) tc->scope = tc->scope->parent;
}

static void define(TypeChecker *tc, char *name, Type *type, bool is_const, SourceLoc loc) {
    for (Symbol *s = tc->scope->symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            diag_error(tc->diag, loc, "GHL201", "redefinition of '%s'", name);
            return;
        }
    }
    Symbol *sym = arena_alloc(tc->arena, sizeof(Symbol));
    sym->name = name;
    sym->type = type;
    sym->is_const = is_const;
    sym->next = tc->scope->symbols;
    tc->scope->symbols = sym;
}

static Symbol *lookup(TypeChecker *tc, const char *name) {
    for (Scope *sc = tc->scope; sc; sc = sc->parent) {
        for (Symbol *s = sc->symbols; s; s = s->next) {
            if (strcmp(s->name, name) == 0) return s;
        }
    }
    for (Symbol *s = tc->builtins; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

static void add_builtin(TypeChecker *tc, const char *name, Type *ret) {
    Symbol *s = arena_alloc(tc->arena, sizeof(Symbol));
    s->name = (char *)name;
    s->type = ret; /* simplified: treat as function returning ret */
    s->is_const = true;
    s->next = tc->builtins;
    tc->builtins = s;
}

static Type *check_expr(TypeChecker *tc, Expr *e);

static Type *check_binary(TypeChecker *tc, Expr *e) {
    Type *lt = check_expr(tc, e->binary.left);
    Type *rt = check_expr(tc, e->binary.right);
    switch (e->binary.op) {
        case BIN_ADD: case BIN_SUB: case BIN_MUL: case BIN_DIV: case BIN_MOD:
            if (lt->kind == TYPE_INT && rt->kind == TYPE_INT) return type_int(tc);
            if (lt->kind == TYPE_FLOAT || rt->kind == TYPE_FLOAT) return type_new(tc->arena, TYPE_FLOAT);
            diag_error(tc->diag, e->loc, "GHL202", "invalid operands for arithmetic operator");
            return type_error(tc);
        case BIN_EQ: case BIN_NE: case BIN_LT: case BIN_GT: case BIN_LE: case BIN_GE:
            return type_bool(tc);
        case BIN_AND: case BIN_OR:
            return type_bool(tc);
        default:
            return type_int(tc);
    }
}

static Type *check_expr(TypeChecker *tc, Expr *e) {
    if (!e) return type_error(tc);
    switch (e->kind) {
        case EXPR_INT:
            e->type = type_int(tc);
            return e->type;
        case EXPR_FLOAT:
            e->type = type_new(tc->arena, TYPE_FLOAT);
            return e->type;
        case EXPR_STRING:
            e->type = type_string(tc);
            return e->type;
        case EXPR_CHAR:
            e->type = type_new(tc->arena, TYPE_CHAR);
            return e->type;
        case EXPR_BOOL:
            e->type = type_bool(tc);
            return e->type;
        case EXPR_NULL:
            e->type = type_ptr(tc->arena, type_void(tc));
            return e->type;
        case EXPR_IDENT: {
            Symbol *s = lookup(tc, e->ident);
            if (!s) {
                diag_error(tc->diag, e->loc, "GHL102", "undefined variable '%s'", e->ident);
                diag_help(tc->diag, "declare '%s' before using it", e->ident);
                e->type = type_error(tc);
                return e->type;
            }
            e->type = s->type;
            return e->type;
        }
        case EXPR_BINARY:
            e->type = check_binary(tc, e);
            return e->type;
        case EXPR_UNARY: {
            Type *ot = check_expr(tc, e->unary.operand);
            switch (e->unary.op) {
                case UN_NEG: case UN_BIT_NOT:
                    e->type = ot;
                    break;
                case UN_NOT:
                    e->type = type_bool(tc);
                    break;
                case UN_ADDR:
                    e->type = type_ptr(tc->arena, ot);
                    break;
                case UN_DEREF:
                    if (ot->kind == TYPE_PTR) e->type = ot->base;
                    else {
                        diag_error(tc->diag, e->loc, "GHL203", "cannot dereference non-pointer");
                        e->type = type_error(tc);
                    }
                    break;
                default:
                    e->type = ot;
            }
            return e->type;
        }
        case EXPR_CALL: {
            /* simplified: check callee is ident, look up */
            if (e->call.callee->kind != EXPR_IDENT) {
                diag_error(tc->diag, e->loc, "GHL204", "can only call named functions in Phase 1");
                e->type = type_error(tc);
                return e->type;
            }
            const char *name = e->call.callee->ident;
            for (int i = 0; i < e->call.arg_count; i++)
                check_expr(tc, e->call.args[i]);

            /* builtins */
            if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0) {
                e->type = type_void(tc);
                return e->type;
            }
            if (strcmp(name, "print_int") == 0) {
                e->type = type_void(tc);
                return e->type;
            }
            if (strcmp(name, "print_float") == 0) {
                e->type = type_void(tc);
                return e->type;
            }
            /* user functions - look in program later; for now assume int */
            Symbol *s = lookup(tc, name);
            if (s) {
                e->type = s->type;
                return e->type;
            }
            diag_error(tc->diag, e->loc, "GHL205", "undefined function '%s'", name);
            e->type = type_error(tc);
            return e->type;
        }
        case EXPR_ASSIGN: {
            Type *tt = check_expr(tc, e->assign.target);
            Type *vt = check_expr(tc, e->assign.value);
            if (e->assign.target->kind == EXPR_IDENT) {
                Symbol *s = lookup(tc, e->assign.target->ident);
                if (s && s->is_const) {
                    diag_error(tc->diag, e->loc, "GHL206", "cannot assign to const '%s'", s->name);
                }
            }
            if (!type_eq(tt, vt) && tt->kind != TYPE_ERROR && vt->kind != TYPE_ERROR) {
                diag_error(tc->diag, e->loc, "GHL201", "type mismatch: cannot assign %s to %s",
                           type_to_str(vt), type_to_str(tt));
            }
            e->type = tt;
            return e->type;
        }
        default:
            e->type = type_error(tc);
            return e->type;
    }
}

static void check_stmt(TypeChecker *tc, Stmt *s);

static void check_stmt(TypeChecker *tc, Stmt *s) {
    if (!s) return;
    switch (s->kind) {
        case STMT_EXPR:
            check_expr(tc, s->expr);
            break;
        case STMT_VAR_DECL: {
            if (s->var_decl.init) {
                Type *it = check_expr(tc, s->var_decl.init);
                if (!type_eq(s->var_decl.type, it) && it->kind != TYPE_ERROR) {
                    diag_error(tc->diag, s->loc, "GHL201",
                               "type mismatch: expected %s, found %s",
                               type_to_str(s->var_decl.type), type_to_str(it));
                }
            }
            define(tc, s->var_decl.name, s->var_decl.type, s->var_decl.is_const, s->loc);
            break;
        }
        case STMT_RETURN: {
            Type *rt = tc->current_func ? tc->current_func->ret_type : type_void(tc);
            if (s->ret_expr) {
                Type *et = check_expr(tc, s->ret_expr);
                if (!type_eq(rt, et) && et->kind != TYPE_ERROR) {
                    diag_error(tc->diag, s->loc, "GHL207",
                               "return type mismatch: expected %s, found %s",
                               type_to_str(rt), type_to_str(et));
                }
            } else if (rt->kind != TYPE_VOID) {
                diag_error(tc->diag, s->loc, "GHL208", "missing return value");
            }
            break;
        }
        case STMT_IF:
            check_expr(tc, s->if_stmt.cond);
            check_stmt(tc, s->if_stmt.then_branch);
            if (s->if_stmt.else_branch) check_stmt(tc, s->if_stmt.else_branch);
            break;
        case STMT_WHILE:
            check_expr(tc, s->while_stmt.cond);
            check_stmt(tc, s->while_stmt.body);
            break;
        case STMT_BLOCK:
            push_scope(tc);
            for (int i = 0; i < s->block.count; i++)
                check_stmt(tc, s->block.stmts[i]);
            pop_scope(tc);
            break;
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
        default:
            break;
    }
}

static void check_function(TypeChecker *tc, Function *f) {
    tc->current_func = f;
    push_scope(tc);
    for (int i = 0; i < f->param_count; i++) {
        define(tc, f->params[i].name, f->params[i].type, false, f->params[i].loc);
    }
    /* register function itself for recursion / calls */
    define(tc, f->name, f->ret_type, true, f->loc);
    check_stmt(tc, f->body);
    pop_scope(tc);
    tc->current_func = NULL;
}

void typecheck_program(Program *prog, DiagnosticEngine *diag) {
    TypeChecker tc = {0};
    tc.diag = diag;
    tc.arena = prog->arena;
    tc.scope = NULL;
    tc.builtins = NULL;

    push_scope(&tc);
    /* register all functions first */
    for (int i = 0; i < prog->func_count; i++) {
        Function *f = prog->funcs[i];
        define(&tc, f->name, f->ret_type, true, f->loc);
    }
    for (int i = 0; i < prog->func_count; i++) {
        check_function(&tc, prog->funcs[i]);
    }
    pop_scope(&tc);
}
