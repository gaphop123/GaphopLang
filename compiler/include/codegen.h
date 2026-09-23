#ifndef GHL_CODEGEN_H
#define GHL_CODEGEN_H

#include "ast.h"
#include "diagnostics.h"

/* Generate C source from GHL AST. Returns heap-allocated string (caller frees). */
char *codegen_to_c(Program *prog, DiagnosticEngine *diag);

#endif
