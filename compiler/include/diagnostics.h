#ifndef GHL_DIAGNOSTICS_H
#define GHL_DIAGNOSTICS_H

#include "common.h"

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_HELP
} DiagLevel;

typedef struct SourceLoc {
    const char *filename;
    int line;
    int column;
} SourceLoc;

typedef struct Diagnostic {
    DiagLevel level;
    const char *code;       /* e.g. "GHL102" */
    SourceLoc loc;
    char *message;
    char *help;
    char *source_line;      /* the line of source */
    int caret_start;
    int caret_end;
    struct Diagnostic *next;
} Diagnostic;

typedef struct {
    Diagnostic *head;
    Diagnostic *tail;
    int error_count;
    int warning_count;
    bool warnings_as_errors;
    bool no_warnings;
} DiagnosticEngine;

void diag_init(DiagnosticEngine *eng);
void diag_free(DiagnosticEngine *eng);

void diag_error(DiagnosticEngine *eng, SourceLoc loc, const char *code,
                const char *fmt, ...);
void diag_warning(DiagnosticEngine *eng, SourceLoc loc, const char *code,
                  const char *fmt, ...);
void diag_note(DiagnosticEngine *eng, SourceLoc loc, const char *fmt, ...);
void diag_help(DiagnosticEngine *eng, const char *fmt, ...);

/* Attach source line + caret to the last diagnostic */
void diag_set_source(DiagnosticEngine *eng, const char *line,
                     int caret_start, int caret_end);

void diag_print_all(DiagnosticEngine *eng, FILE *out);
bool diag_has_errors(DiagnosticEngine *eng);

#endif
