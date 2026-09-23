#ifndef GHL_TYPECHECK_H
#define GHL_TYPECHECK_H

#include "ast.h"
#include "diagnostics.h"

void typecheck_program(Program *prog, DiagnosticEngine *diag);

#endif
