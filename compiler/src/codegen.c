#include "codegen.h"

static void emit_type(StringBuilder *sb, Type *t) {
    if (!t) { sb_append(sb, "void"); return; }
    switch (t->kind) {
        case TYPE_VOID: sb_append(sb, "void"); break;
        case TYPE_INT: case TYPE_LONG: sb_append(sb, "long long"); break;
        case TYPE_UINT: case TYPE_ULONG: sb_append(sb, "unsigned long long"); break;
        case TYPE_SHORT: sb_append(sb, "short"); break;
        case TYPE_USHORT: sb_append(sb, "unsigned short"); break;
        case TYPE_FLOAT: sb_append(sb, "float"); break;
        case TYPE_DOUBLE: sb_append(sb, "double"); break;
        case TYPE_CHAR: case TYPE_BYTE: sb_append(sb, "char"); break;
        case TYPE_BOOL: sb_append(sb, "int"); break;
        case TYPE_STRING: sb_append(sb, "const char*"); break;
        case TYPE_PTR:
            emit_type(sb, t->base);
            sb_append(sb, "*");
            break;
        default: sb_append(sb, "void"); break;
    }
}

static void emit_expr(StringBuilder *sb, Expr *e);

static void emit_binop(StringBuilder *sb, BinOp op) {
    switch (op) {
        case BIN_ADD: sb_append(sb, "+"); break;
        case BIN_SUB: sb_append(sb, "-"); break;
        case BIN_MUL: sb_append(sb, "*"); break;
        case BIN_DIV: sb_append(sb, "/"); break;
        case BIN_MOD: sb_append(sb, "%"); break;
        case BIN_EQ: sb_append(sb, "=="); break;
        case BIN_NE: sb_append(sb, "!="); break;
        case BIN_LT: sb_append(sb, "<"); break;
        case BIN_GT: sb_append(sb, ">"); break;
        case BIN_LE: sb_append(sb, "<="); break;
        case BIN_GE: sb_append(sb, ">="); break;
        case BIN_AND: sb_append(sb, "&&"); break;
        case BIN_OR: sb_append(sb, "||"); break;
        case BIN_BIT_AND: sb_append(sb, "&"); break;
        case BIN_BIT_OR: sb_append(sb, "|"); break;
        case BIN_BIT_XOR: sb_append(sb, "^"); break;
        case BIN_SHL: sb_append(sb, "<<"); break;
        case BIN_SHR: sb_append(sb, ">>"); break;
    }
}

static void emit_expr(StringBuilder *sb, Expr *e) {
    if (!e) return;
    switch (e->kind) {
        case EXPR_INT:
            sb_appendf(sb, "%lld", (long long)e->int_val);
            break;
        case EXPR_FLOAT:
            sb_appendf(sb, "%g", e->float_val);
            break;
        case EXPR_STRING: {
            sb_append(sb, "\"");
            for (const char *p = e->str_val; *p; p++) {
                if (*p == '\n') sb_append(sb, "\\n");
                else if (*p == '\t') sb_append(sb, "\\t");
                else if (*p == '"') sb_append(sb, "\\\"");
                else if (*p == '\\') sb_append(sb, "\\\\");
                else {
                    char t[2] = {*p, 0};
                    sb_append(sb, t);
                }
            }
            sb_append(sb, "\"");
            break;
        }
        case EXPR_CHAR:
            sb_appendf(sb, "'%c'", e->char_val);
            break;
        case EXPR_BOOL:
            sb_append(sb, e->bool_val ? "1" : "0");
            break;
        case EXPR_NULL:
            sb_append(sb, "NULL");
            break;
        case EXPR_IDENT:
            sb_append(sb, e->ident);
            break;
        case EXPR_BINARY:
            sb_append(sb, "(");
            emit_expr(sb, e->binary.left);
            sb_append(sb, " ");
            emit_binop(sb, e->binary.op);
            sb_append(sb, " ");
            emit_expr(sb, e->binary.right);
            sb_append(sb, ")");
            break;
        case EXPR_UNARY:
            switch (e->unary.op) {
                case UN_NEG: sb_append(sb, "-"); emit_expr(sb, e->unary.operand); break;
                case UN_NOT: sb_append(sb, "!"); emit_expr(sb, e->unary.operand); break;
                case UN_BIT_NOT: sb_append(sb, "~"); emit_expr(sb, e->unary.operand); break;
                case UN_ADDR: sb_append(sb, "&"); emit_expr(sb, e->unary.operand); break;
                case UN_DEREF: sb_append(sb, "*"); emit_expr(sb, e->unary.operand); break;
                case UN_PRE_INC: sb_append(sb, "++"); emit_expr(sb, e->unary.operand); break;
                case UN_PRE_DEC: sb_append(sb, "--"); emit_expr(sb, e->unary.operand); break;
                case UN_POST_INC: emit_expr(sb, e->unary.operand); sb_append(sb, "++"); break;
                case UN_POST_DEC: emit_expr(sb, e->unary.operand); sb_append(sb, "--"); break;
            }
            break;
        case EXPR_CALL:
            if (e->call.callee->kind == EXPR_IDENT) {
                const char *name = e->call.callee->ident;
                if (strcmp(name, "print") == 0) {
                    sb_append(sb, "printf(\"%s\"");
                    for (int i = 0; i < e->call.arg_count; i++) {
                        sb_append(sb, ", ");
                        emit_expr(sb, e->call.args[i]);
                    }
                    sb_append(sb, ")");
                } else if (strcmp(name, "println") == 0) {
                    sb_append(sb, "printf(\"%s\\n\"");
                    for (int i = 0; i < e->call.arg_count; i++) {
                        sb_append(sb, ", ");
                        emit_expr(sb, e->call.args[i]);
                    }
                    sb_append(sb, ")");
                } else if (strcmp(name, "print_int") == 0) {
                    sb_append(sb, "printf(\"%lld\\n\"");
                    if (e->call.arg_count > 0) {
                        sb_append(sb, ", (long long)");
                        emit_expr(sb, e->call.args[0]);
                    }
                    sb_append(sb, ")");
                } else if (strcmp(name, "print_float") == 0) {
                    sb_append(sb, "printf(\"%g\\n\"");
                    if (e->call.arg_count > 0) {
                        sb_append(sb, ", ");
                        emit_expr(sb, e->call.args[0]);
                    }
                    sb_append(sb, ")");
                } else {
                    sb_append(sb, name);
                    sb_append(sb, "(");
                    for (int i = 0; i < e->call.arg_count; i++) {
                        if (i) sb_append(sb, ", ");
                        emit_expr(sb, e->call.args[i]);
                    }
                    sb_append(sb, ")");
                }
            }
            break;
        case EXPR_ASSIGN:
            emit_expr(sb, e->assign.target);
            sb_append(sb, " = ");
            emit_expr(sb, e->assign.value);
            break;
        default:
            sb_append(sb, "0");
    }
}

static void emit_stmt(StringBuilder *sb, Stmt *s, int indent);

static void emit_indent(StringBuilder *sb, int n) {
    for (int i = 0; i < n; i++) sb_append(sb, "    ");
}

static void emit_stmt(StringBuilder *sb, Stmt *s, int indent) {
    if (!s) return;
    switch (s->kind) {
        case STMT_EXPR:
            emit_indent(sb, indent);
            emit_expr(sb, s->expr);
            sb_append(sb, ";\n");
            break;
        case STMT_VAR_DECL:
            emit_indent(sb, indent);
            emit_type(sb, s->var_decl.type);
            sb_append(sb, " ");
            sb_append(sb, s->var_decl.name);
            if (s->var_decl.init) {
                sb_append(sb, " = ");
                emit_expr(sb, s->var_decl.init);
            }
            sb_append(sb, ";\n");
            break;
        case STMT_RETURN:
            emit_indent(sb, indent);
            sb_append(sb, "return");
            if (s->ret_expr) {
                sb_append(sb, " ");
                emit_expr(sb, s->ret_expr);
            }
            sb_append(sb, ";\n");
            break;
        case STMT_IF:
            emit_indent(sb, indent);
            sb_append(sb, "if (");
            emit_expr(sb, s->if_stmt.cond);
            sb_append(sb, ") {\n");
            emit_stmt(sb, s->if_stmt.then_branch, indent + 1);
            emit_indent(sb, indent);
            sb_append(sb, "}");
            if (s->if_stmt.else_branch) {
                sb_append(sb, " else {\n");
                emit_stmt(sb, s->if_stmt.else_branch, indent + 1);
                emit_indent(sb, indent);
                sb_append(sb, "}");
            }
            sb_append(sb, "\n");
            break;
        case STMT_WHILE:
            emit_indent(sb, indent);
            sb_append(sb, "while (");
            emit_expr(sb, s->while_stmt.cond);
            sb_append(sb, ") {\n");
            emit_stmt(sb, s->while_stmt.body, indent + 1);
            emit_indent(sb, indent);
            sb_append(sb, "}\n");
            break;
        case STMT_BLOCK:
            for (int i = 0; i < s->block.count; i++)
                emit_stmt(sb, s->block.stmts[i], indent);
            break;
        case STMT_BREAK:
            emit_indent(sb, indent);
            sb_append(sb, "break;\n");
            break;
        case STMT_CONTINUE:
            emit_indent(sb, indent);
            sb_append(sb, "continue;\n");
            break;
        default:
            break;
    }
}

static void emit_function(StringBuilder *sb, Function *f) {
    emit_type(sb, f->ret_type);
    sb_append(sb, " ");
    /* main must be main for C */
    if (strcmp(f->name, "main") == 0)
        sb_append(sb, "main");
    else
        sb_append(sb, f->name);
    sb_append(sb, "(");
    if (f->param_count == 0) {
        sb_append(sb, "void");
    } else {
        for (int i = 0; i < f->param_count; i++) {
            if (i) sb_append(sb, ", ");
            emit_type(sb, f->params[i].type);
            sb_append(sb, " ");
            sb_append(sb, f->params[i].name);
        }
    }
    sb_append(sb, ") {\n");
    emit_stmt(sb, f->body, 1);
    sb_append(sb, "}\n\n");
}

char *codegen_to_c(Program *prog, DiagnosticEngine *diag) {
    (void)diag;
    StringBuilder sb;
    sb_init(&sb);

    sb_append(&sb, "/* Generated by GaphopLang Compiler " GHL_VERSION " */\n");
    sb_append(&sb, "#include <stdio.h>\n");
    sb_append(&sb, "#include <stdlib.h>\n");
    sb_append(&sb, "#include <string.h>\n");
    sb_append(&sb, "#include <stdbool.h>\n\n");

    for (int i = 0; i < prog->func_count; i++) {
        emit_function(&sb, prog->funcs[i]);
    }

    return sb_to_string(&sb);
}
