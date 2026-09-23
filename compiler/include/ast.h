#ifndef GHL_AST_H
#define GHL_AST_H

#include "common.h"
#include "diagnostics.h"

typedef enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_UINT,
    TYPE_SHORT,
    TYPE_USHORT,
    TYPE_LONG,
    TYPE_ULONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_BYTE,
    TYPE_PTR,       /* pointer to base */
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNKNOWN,
    TYPE_ERROR
} TypeKind;

typedef struct Type {
    TypeKind kind;
    struct Type *base;      /* for PTR / ARRAY */
    int array_size;         /* -1 if unknown */
    const char *name;       /* for named types / structs */
} Type;

typedef enum {
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STRING,
    EXPR_CHAR,
    EXPR_BOOL,
    EXPR_NULL,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_ASSIGN,
    EXPR_INDEX,
    EXPR_MEMBER,
    EXPR_CAST
} ExprKind;

typedef enum {
    BIN_ADD, BIN_SUB, BIN_MUL, BIN_DIV, BIN_MOD,
    BIN_EQ, BIN_NE, BIN_LT, BIN_GT, BIN_LE, BIN_GE,
    BIN_AND, BIN_OR,
    BIN_BIT_AND, BIN_BIT_OR, BIN_BIT_XOR, BIN_SHL, BIN_SHR
} BinOp;

typedef enum {
    UN_NEG, UN_NOT, UN_BIT_NOT, UN_ADDR, UN_DEREF, UN_PRE_INC, UN_PRE_DEC,
    UN_POST_INC, UN_POST_DEC
} UnOp;

typedef struct Expr {
    ExprKind kind;
    SourceLoc loc;
    Type *type;             /* filled by typechecker */
    union {
        int64_t int_val;
        double float_val;
        char *str_val;
        char char_val;
        bool bool_val;
        char *ident;
        struct {
            BinOp op;
            struct Expr *left;
            struct Expr *right;
        } binary;
        struct {
            UnOp op;
            struct Expr *operand;
        } unary;
        struct {
            struct Expr *callee;
            struct Expr **args;
            int arg_count;
        } call;
        struct {
            struct Expr *target;
            struct Expr *value;
        } assign;
        struct {
            struct Expr *array;
            struct Expr *index;
        } index;
        struct {
            struct Expr *object;
            char *member;
        } member;
        struct {
            Type *to;
            struct Expr *expr;
        } cast;
    };
} Expr;

typedef enum {
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_BREAK,
    STMT_CONTINUE
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    SourceLoc loc;
    union {
        Expr *expr;
        struct {
            Type *type;
            char *name;
            Expr *init;
            bool is_const;
        } var_decl;
        Expr *ret_expr;     /* may be NULL for void */
        struct {
            Expr *cond;
            struct Stmt *then_branch;
            struct Stmt *else_branch;
        } if_stmt;
        struct {
            Expr *cond;
            struct Stmt *body;
        } while_stmt;
        struct {
            struct Stmt *init;
            Expr *cond;
            Expr *step;
            struct Stmt *body;
        } for_stmt;
        struct {
            struct Stmt **stmts;
            int count;
        } block;
    };
} Stmt;

typedef struct Param {
    Type *type;
    char *name;
    SourceLoc loc;
} Param;

typedef struct Function {
    char *name;
    Type *ret_type;
    Param *params;
    int param_count;
    Stmt *body;
    SourceLoc loc;
} Function;

typedef struct Program {
    Function **funcs;
    int func_count;
    Arena *arena;
} Program;

/* Constructors (all allocate in arena) */
Type *type_new(Arena *a, TypeKind kind);
Type *type_ptr(Arena *a, Type *base);
const char *type_to_str(Type *t);

Expr *expr_int(Arena *a, SourceLoc loc, int64_t v);
Expr *expr_float(Arena *a, SourceLoc loc, double v);
Expr *expr_string(Arena *a, SourceLoc loc, char *s);
Expr *expr_bool(Arena *a, SourceLoc loc, bool v);
Expr *expr_char(Arena *a, SourceLoc loc, char v);
Expr *expr_char(Arena *a, SourceLoc loc, char v);
Expr *expr_null(Arena *a, SourceLoc loc);
Expr *expr_ident(Arena *a, SourceLoc loc, char *name);
Expr *expr_binary(Arena *a, SourceLoc loc, BinOp op, Expr *l, Expr *r);
Expr *expr_unary(Arena *a, SourceLoc loc, UnOp op, Expr *operand);
Expr *expr_call(Arena *a, SourceLoc loc, Expr *callee, Expr **args, int n);
Expr *expr_assign(Arena *a, SourceLoc loc, Expr *target, Expr *value);

Stmt *stmt_expr(Arena *a, SourceLoc loc, Expr *e);
Stmt *stmt_var_decl(Arena *a, SourceLoc loc, Type *t, char *name, Expr *init, bool is_const);
Stmt *stmt_return(Arena *a, SourceLoc loc, Expr *e);
Stmt *stmt_if(Arena *a, SourceLoc loc, Expr *cond, Stmt *then_b, Stmt *else_b);
Stmt *stmt_while(Arena *a, SourceLoc loc, Expr *cond, Stmt *body);
Stmt *stmt_block(Arena *a, SourceLoc loc, Stmt **stmts, int n);
Stmt *stmt_break(Arena *a, SourceLoc loc);
Stmt *stmt_continue(Arena *a, SourceLoc loc);

Function *func_new(Arena *a, SourceLoc loc, char *name, Type *ret, Param *params, int n, Stmt *body);
Program *program_new(Arena *a);

#endif
