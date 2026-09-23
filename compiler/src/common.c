#include "common.h"

Arena *arena_create(size_t size) {
    Arena *a = malloc(sizeof(Arena));
    if (!a) return NULL;
    a->buf = malloc(size);
    if (!a->buf) { free(a); return NULL; }
    a->size = size;
    a->used = 0;
    return a;
}

void *arena_alloc(Arena *a, size_t size) {
    /* Align to 8 bytes */
    size = (size + 7) & ~((size_t)7);
    if (a->used + size > a->size) {
        /* Grow */
        size_t new_size = a->size * 2;
        while (a->used + size > new_size) new_size *= 2;
        char *nb = realloc(a->buf, new_size);
        if (!nb) return NULL;
        a->buf = nb;
        a->size = new_size;
    }
    void *p = a->buf + a->used;
    a->used += size;
    return p;
}

void arena_reset(Arena *a) {
    a->used = 0;
}

void arena_destroy(Arena *a) {
    if (a) {
        free(a->buf);
        free(a);
    }
}

char *arena_strdup(Arena *a, const char *s) {
    size_t n = strlen(s) + 1;
    char *p = arena_alloc(a, n);
    if (p) memcpy(p, s, n);
    return p;
}

void sb_init(StringBuilder *sb) {
    sb->data = malloc(64);
    sb->len = 0;
    sb->cap = 64;
    if (sb->data) sb->data[0] = '\0';
}

void sb_free(StringBuilder *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

void sb_append(StringBuilder *sb, const char *s) {
    size_t n = strlen(s);
    if (sb->len + n + 1 > sb->cap) {
        size_t nc = sb->cap * 2;
        while (sb->len + n + 1 > nc) nc *= 2;
        char *nd = realloc(sb->data, nc);
        if (!nd) return;
        sb->data = nd;
        sb->cap = nc;
    }
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_appendf(StringBuilder *sb, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    sb_append(sb, buf);
}

char *sb_to_string(StringBuilder *sb) {
    char *r = sb->data;
    sb->data = NULL;
    sb->len = sb->cap = 0;
    return r;
}
