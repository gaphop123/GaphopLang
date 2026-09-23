#include "diagnostics.h"

void diag_init(DiagnosticEngine *eng) {
    eng->head = eng->tail = NULL;
    eng->error_count = 0;
    eng->warning_count = 0;
    eng->warnings_as_errors = false;
    eng->no_warnings = false;
}

static void free_diag(Diagnostic *d) {
    while (d) {
        Diagnostic *n = d->next;
        free(d->message);
        free(d->help);
        free(d->source_line);
        free(d);
        d = n;
    }
}

void diag_free(DiagnosticEngine *eng) {
    free_diag(eng->head);
    eng->head = eng->tail = NULL;
}

static Diagnostic *diag_new(DiagLevel level, SourceLoc loc, const char *code) {
    Diagnostic *d = calloc(1, sizeof(Diagnostic));
    if (!d) return NULL;
    d->level = level;
    d->code = code;
    d->loc = loc;
    return d;
}

static void diag_push(DiagnosticEngine *eng, Diagnostic *d) {
    if (!eng->head) eng->head = d;
    else eng->tail->next = d;
    eng->tail = d;
}

void diag_error(DiagnosticEngine *eng, SourceLoc loc, const char *code,
                const char *fmt, ...) {
    Diagnostic *d = diag_new(DIAG_ERROR, loc, code);
    if (!d) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    d->message = strdup(buf);
    eng->error_count++;
    diag_push(eng, d);
}

void diag_warning(DiagnosticEngine *eng, SourceLoc loc, const char *code,
                  const char *fmt, ...) {
    if (eng->no_warnings) return;
    Diagnostic *d = diag_new(eng->warnings_as_errors ? DIAG_ERROR : DIAG_WARNING,
                             loc, code);
    if (!d) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    d->message = strdup(buf);
    if (eng->warnings_as_errors) eng->error_count++;
    else eng->warning_count++;
    diag_push(eng, d);
}

void diag_note(DiagnosticEngine *eng, SourceLoc loc, const char *fmt, ...) {
    Diagnostic *d = diag_new(DIAG_NOTE, loc, NULL);
    if (!d) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    d->message = strdup(buf);
    diag_push(eng, d);
}

void diag_help(DiagnosticEngine *eng, const char *fmt, ...) {
    if (!eng->tail) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    free(eng->tail->help);
    eng->tail->help = strdup(buf);
}

void diag_set_source(DiagnosticEngine *eng, const char *line,
                     int caret_start, int caret_end) {
    if (!eng->tail) return;
    free(eng->tail->source_line);
    eng->tail->source_line = line ? strdup(line) : NULL;
    eng->tail->caret_start = caret_start;
    eng->tail->caret_end = caret_end;
}

static const char *level_str(DiagLevel l) {
    switch (l) {
        case DIAG_ERROR:   return "error";
        case DIAG_WARNING: return "warning";
        case DIAG_NOTE:    return "note";
        case DIAG_HELP:    return "help";
    }
    return "diag";
}

void diag_print_all(DiagnosticEngine *eng, FILE *out) {
    for (Diagnostic *d = eng->head; d; d = d->next) {
        if (d->code)
            fprintf(out, "%s[%s]: %s\n", level_str(d->level), d->code, d->message);
        else
            fprintf(out, "%s: %s\n", level_str(d->level), d->message);

        if (d->loc.filename) {
            fprintf(out, "  --> %s:%d:%d\n", d->loc.filename, d->loc.line, d->loc.column);
        }

        if (d->source_line) {
            fprintf(out, "   |\n");
            fprintf(out, "%4d | %s\n", d->loc.line, d->source_line);
            fprintf(out, "   | ");
            for (int i = 1; i < d->caret_start; i++) fputc(' ', out);
            for (int i = d->caret_start; i <= d->caret_end; i++) fputc('^', out);
            fputc('\n', out);
        }

        if (d->help) {
            fprintf(out, "   |\n");
            fprintf(out, "   = help: %s\n", d->help);
        }
        fputc('\n', out);
    }
}

bool diag_has_errors(DiagnosticEngine *eng) {
    return eng->error_count > 0;
}
