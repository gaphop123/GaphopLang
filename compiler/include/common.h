#ifndef GHL_COMMON_H
#define GHL_COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

#define GHL_VERSION "0.1.0"
#define GHL_LANG_VERSION "0.1"

/* Arena allocator for AST / temporary data */
typedef struct Arena {
    char *buf;
    size_t size;
    size_t used;
} Arena;

Arena *arena_create(size_t size);
void *arena_alloc(Arena *a, size_t size);
void arena_reset(Arena *a);
void arena_destroy(Arena *a);
char *arena_strdup(Arena *a, const char *s);

/* Dynamic string builder */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StringBuilder;

void sb_init(StringBuilder *sb);
void sb_free(StringBuilder *sb);
void sb_append(StringBuilder *sb, const char *s);
void sb_appendf(StringBuilder *sb, const char *fmt, ...);
char *sb_to_string(StringBuilder *sb); /* takes ownership of data */

#endif /* GHL_COMMON_H */
