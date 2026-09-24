#include "common.h"
#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "typecheck.h"
#include "codegen.h"

#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#  define mkdir(path, mode) _mkdir(path)
#  define access _access
#  define F_OK 0
#  define R_OK 4
#else
#  include <unistd.h>
#endif

#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
#  define GHL_ON_WINDOWS 1
#else
#  define GHL_ON_WINDOWS 0
#endif


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
    return mkdir(path, 0755);
}


static void merge_program(Program *dst, Program *src) {
    if (src->func_count > 0) {
        Function **nf = arena_alloc(dst->arena, sizeof(Function *) * (dst->func_count + src->func_count));
        for (int i = 0; i < dst->func_count; i++) nf[i] = dst->funcs[i];
        for (int i = 0; i < src->func_count; i++) nf[dst->func_count + i] = src->funcs[i];
        dst->funcs = nf;
        dst->func_count += src->func_count;
    }
    if (src->struct_count > 0) {
        StructDef **ns = arena_alloc(dst->arena, sizeof(StructDef *) * (dst->struct_count + src->struct_count));
        for (int i = 0; i < dst->struct_count; i++) ns[i] = dst->structs[i];
        for (int i = 0; i < src->struct_count; i++) ns[dst->struct_count + i] = src->structs[i];
        dst->structs = ns;
        dst->struct_count += src->struct_count;
    }
}

static int resolve_module_path(const char *imp, char *out, size_t outsz) {
    if (!imp || !imp[0]) return -1;
    if (imp[0] == '.' || strchr(imp, '/') || strstr(imp, ".ghl")) {
        snprintf(out, outsz, "%s", imp);
        return access(out, R_OK) == 0 ? 0 : -1;
    }
    snprintf(out, outsz, "std/%s.ghl", imp);
    if (access(out, R_OK) == 0) return 0;
    const char *stdroot = getenv("GHL_STD");
    if (stdroot) {
        snprintf(out, outsz, "%s/%s.ghl", stdroot, imp);
        if (access(out, R_OK) == 0) return 0;
    }
    /* try alongside executable ../../std */
    snprintf(out, outsz, "../std/%s.ghl", imp);
    if (access(out, R_OK) == 0) return 0;
    snprintf(out, outsz, "std/%s.ghl", imp);
    return -1;
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

    /* Resolve imports (one level, no cycles for Phase 2) */
    for (int ii = 0; ii < prog->import_count; ii++) {
        char modpath[1024];
        ImportDecl *imp = prog->imports[ii];
        if (resolve_module_path(imp->path, modpath, sizeof(modpath)) != 0) {
            diag_error(&diag, imp->loc, "GHL001", "module not found: %s", imp->path);
            diag_help(&diag, "looked for std/%s.ghl — set GHL_STD or use a relative path", imp->path);
            continue;
        }
        printf("      import %s -> %s\n", imp->path, modpath);
        size_t mlen = 0;
        char *msrc = read_file(modpath, &mlen);
        if (!msrc) {
            diag_error(&diag, imp->loc, "GHL001", "cannot read module %s", modpath);
            continue;
        }
        Lexer mlex;
        lexer_init(&mlex, msrc, mlen, modpath, &diag, arena);
        Parser mparser;
        parser_init(&mparser, &mlex, &diag, arena);
        Program *mprog = parse_program(&mparser);
        if (mprog) merge_program(prog, mprog);
        /* keep msrc alive for lexemes (arena lifetime ends at end of compile) */
        (void)msrc;
    }

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
    char default_out[512];
#if GHL_ON_WINDOWS
    snprintf(default_out, sizeof(default_out), "build/a.exe");
#else
    snprintf(default_out, sizeof(default_out), "build/a.out");
#endif
    const char *exe = out_exe ? out_exe : default_out;
    snprintf(cmd, sizeof(cmd), "gcc -O2 -o \"%s\" \"%s\" -lm 2>&1", exe, c_path);
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

static const char *basename_of(const char *path) {
    const char *s = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') s = p + 1;
    }
    return s;
}

static int cmd_new(const char *name) {
    if (ensure_dir(name) != 0) {
        fprintf(stderr, "error: cannot create project directory '%s'\n", name);
        return 1;
    }
    const char *base = basename_of(name);
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
        fprintf(f, "name = \"%s\"\n", base);
        fprintf(f, "version = \"0.1.0\"\n");
        fprintf(f, "entry = \"src/main.ghl\"\n");
        #if GHL_ON_WINDOWS
        fprintf(f, "output = \"bin/%s.exe\"\n", base);
#else
        fprintf(f, "output = \"bin/%s\"\n", base);
#endif
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


/* Very small TOML reader for ghl.toml: name / entry / output */
typedef struct {
    char name[256];
    char entry[512];
    char output[512];
} ProjectConfig;

static int load_ghl_toml(const char *path, ProjectConfig *cfg) {
    cfg->name[0] = cfg->entry[0] = cfg->output[0] = '\0';
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char key[64], val[512];
        if (sscanf(p, "%63[^= \t] = \"%511[^\"]\"", key, val) == 2 ||
            sscanf(p, "%63[^= \t]=\"%511[^\"]\"", key, val) == 2) {
            if (strcmp(key, "name") == 0) snprintf(cfg->name, sizeof(cfg->name), "%s", val);
            else if (strcmp(key, "entry") == 0) snprintf(cfg->entry, sizeof(cfg->entry), "%s", val);
            else if (strcmp(key, "output") == 0) snprintf(cfg->output, sizeof(cfg->output), "%s", val);
        }
    }
    fclose(f);
    return 1;
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
#if GHL_ON_WINDOWS
    printf("Target: Windows x64 (MSYS2 / MinGW UCRT64)\n");
#else
    printf("Target: Linux x86-64\n");
#endif
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

    static char src_buf[512], out_buf[512];
    if (!src) {
        ProjectConfig cfg;
        if (load_ghl_toml("ghl.toml", &cfg)) {
            if (cfg.entry[0]) {
                snprintf(src_buf, sizeof(src_buf), "%s", cfg.entry);
                src = src_buf;
            }
            if (!out && cfg.output[0]) {
                snprintf(out_buf, sizeof(out_buf), "%s", cfg.output);
                out = out_buf;
                /* ensure parent dir */
                ensure_dir("bin");
            }
        }
        if (!src && access("src/main.ghl", R_OK) == 0) {
            src = "src/main.ghl";
        }
        if (!src && access("main.ghl", R_OK) == 0) {
            src = "main.ghl";
        }
        if (!src) {
            fprintf(stderr, "error: no source file specified and no ghl.toml / src/main.ghl found\n");
            return 1;
        }
        if (!out) {
#if GHL_ON_WINDOWS
            out = "bin/app.exe";
#else
            out = "bin/app";
#endif
            ensure_dir("bin");
        }
    }

    if (!out && !do_check) {
        #if GHL_ON_WINDOWS
        out = "build/a.exe";
#else
        out = "build/a.out";
#endif
        ensure_dir("build");
    }

    return compile_file(src, out, do_run, do_check);
}
