#include "common.h"
#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "typecheck.h"
#include "codegen.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, sz, f);
    buf[n] = '\0';
    fclose(f);
    if (out_len) *out_len = n;
    return buf;
}

static int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        return -1;
    }
    return mkdir(path);
}

static int compile_file(const char *src_path, const char *out_exe, bool run_after, bool check_only) {
    size_t len = 0;
    char *src = read_file(src_path, &len);
    if (!src) {
        fprintf(stderr, "error: cannot read '%s'\n", src_path);
        return 1;
    }

    DiagnosticEngine diag;
    diag_init(&diag);
    Arena *arena = arena_create(1024 * 1024);
    if (!arena) {
        free(src);
        return 1;
    }

    printf("Building %s...\n\n", src_path);
    printf("[1/5] Lexing\n");
    Lexer lex;
    lexer_init(&lex, src, len, src_path, &diag, arena);

    printf("[2/5] Parsing\n");
    Parser parser;
    parser_init(&parser, &lex, &diag, arena);
    Program *prog = parse_program(&parser);

    if (diag_has_errors(&diag)) {
        diag_print_all(&diag, stderr);
        printf("\nBuild failed with %d error(s).\n", diag.error_count);
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 1;
    }

    printf("[3/5] Type checking\n");
    typecheck_program(prog, &diag);

    if (diag_has_errors(&diag)) {
        diag_print_all(&diag, stderr);
        printf("\nBuild failed with %d error(s).\n", diag.error_count);
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 1;
    }

    if (check_only) {
        printf("\nCheck succeeded. No errors found.\n");
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 0;
    }

    printf("[4/5] Code generation\n");
    char *c_code = codegen_to_c(prog, &diag);
    if (!c_code) {
        fprintf(stderr, "codegen failed\n");
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 1;
    }

    ensure_dir("build");
    char c_path[512];
    snprintf(c_path, sizeof(c_path), "build/%s.c", "ghl_out");
    FILE *cf = fopen(c_path, "w");
    if (!cf) {
        fprintf(stderr, "cannot write %s\n", c_path);
        free(c_code);
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 1;
    }
    fputs(c_code, cf);
    fclose(cf);
    free(c_code);

    printf("[5/5] Linking\n");
    char cmd[1024];
    const char *exe = out_exe ? out_exe : "build/a.out";
    snprintf(cmd, sizeof(cmd), "gcc -O2 -o \"%s\" \"%s\" 2>&1", exe, c_path);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "linker/compiler backend failed\n");
        arena_destroy(arena);
        free(src);
        diag_free(&diag);
        return 1;
    }

    printf("\nBuild succeeded.\n\nOutput:\n  %s\n", exe);

    if (run_after) {
        printf("\n--- Running ---\n");
        char runcmd[512];
        snprintf(runcmd, sizeof(runcmd), "\"%s\"", exe);
        system(runcmd);
    }

    arena_destroy(arena);
    free(src);
    diag_free(&diag);
    return 0;
}

static int cmd_new(const char *name) {
    if (ensure_dir(name) != 0) {
        fprintf(stderr, "error: cannot create project directory '%s'\n", name);
        return 1;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/src", name);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/include", name);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/assets", name);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/build", name);
    ensure_dir(path);
    snprintf(path, sizeof(path), "%s/bin", name);
    ensure_dir(path);

    snprintf(path, sizeof(path), "%s/ghl.toml", name);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "name = \"%s\"\n", name);
        fprintf(f, "version = \"0.1.0\"\n");
        fprintf(f, "entry = \"src/main.ghl\"\n");
        fprintf(f, "output = \"bin/%s\"\n", name);
        fclose(f);
    }

    snprintf(path, sizeof(path), "%s/src/main.ghl", name);
    f = fopen(path, "w");
    if (f) {
        fprintf(f, "fn main() -> int {\n");
        fprintf(f, "    println(\"Hello, GaphopLang!\");\n");
        fprintf(f, "    return 0;\n");
        fprintf(f, "}\n");
        fclose(f);
    }

    printf("Created project '%s'\n", name);
    printf("  %s/\n", name);
    printf("  ├── ghl.toml\n");
    printf("  ├── src/\n");
    printf("  │   └── main.ghl\n");
    printf("  ├── include/\n");
    printf("  ├── assets/\n");
    printf("  ├── build/\n");
    printf("  └── bin/\n");
    return 0;
}

static void print_help(void) {
    printf("GaphopLang Compiler %s\n\n", GHL_VERSION);
    printf("Usage:\n");
    printf("    ghlc <command> [options]\n\n");
    printf("Commands:\n");
    printf("    new <name>     Create a new project\n");
    printf("    build [file]   Build project or single file\n");
    printf("    run [file]     Build and run\n");
    printf("    check [file]   Type-check without generating executable\n");
    printf("    clean          Remove build artifacts\n");
    printf("    version        Show version\n");
    printf("    help           Show this help\n\n");
    printf("Options:\n");
    printf("    -o <file>      Output executable path\n");
    printf("    --debug        Debug build\n");
    printf("    --release      Release build (default)\n");
}

static void print_version(void) {
    printf("GaphopLang Compiler %s\n", GHL_VERSION);
    printf("GHL Language %s\n", GHL_LANG_VERSION);
    printf("Target: Linux x86-64\n");
    printf("Backend: C (gcc)\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
        print_version();
        return 0;
    }
    if (strcmp(cmd, "new") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: ghlc new <project-name>\n");
            return 1;
        }
        return cmd_new(argv[2]);
    }
    if (strcmp(cmd, "clean") == 0) {
        system("rm -rf build/* bin/* 2>/dev/null");
        printf("Cleaned build artifacts.\n");
        return 0;
    }

    bool do_run = strcmp(cmd, "run") == 0;
    bool do_check = strcmp(cmd, "check") == 0;
    bool do_build = strcmp(cmd, "build") == 0 || do_run || do_check;

    if (!do_build) {
        fprintf(stderr, "unknown command: %s\n", cmd);
        print_help();
        return 1;
    }

    const char *src = NULL;
    const char *out = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out = argv[++i];
        } else if (argv[i][0] != '-') {
            src = argv[i];
        }
    }

    if (!src) {
        /* look for ghl.toml / src/main.ghl */
        if (access("src/main.ghl", R_OK) == 0) {
            src = "src/main.ghl";
            if (!out) {
                /* try read name from ghl.toml - simplified */
                out = "bin/app";
                ensure_dir("bin");
            }
        } else if (access("main.ghl", R_OK) == 0) {
            src = "main.ghl";
            if (!out) out = "build/main";
        } else {
            fprintf(stderr, "error: no source file specified and no src/main.ghl found\n");
            return 1;
        }
    }

    if (!out && !do_check) {
        out = "build/a.out";
        ensure_dir("build");
    }

    return compile_file(src, out, do_run, do_check);
}
