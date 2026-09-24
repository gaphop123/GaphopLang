# GaphopLang (GHL)

**Explicit. Strict. Native.**

GaphopLang is a native systems programming language with C-like syntax, designed for applications, tools, games, and system utilities.

```
fn main() -> int {
    println("Hello, GaphopLang!");
    return 0;
}
```

## Philosophy

> "Explicit code. Strict errors. Native performance."

- Native programming language
- Compiler written in C
- Source files use `.ghl` extension
- Builds directly to native executables
- Strict type system and error reporting
- Explicit memory management (no GC by default)
- No hidden magic that makes debugging harder

## Current Status: Phase 1–2 (Bootstrap + Modules + Studio)

| Component              | Status          |
|------------------------|-----------------|
| Lexer                  | ✅ Implemented  |
| Parser + AST           | ✅ Implemented  |
| Basic Type Checker     | ✅ Implemented  |
| Codegen (C backend)    | ✅ Implemented  |
| `ghlc build` / `run`   | ✅ Working      |
| Hello World            | ✅ Working      |
| Linux native           | ✅ Working      |
| Windows .exe native    | ✅ Supported (MSYS2 UCRT64) |
| Project system         | ✅ Basic (`ghl.toml`) |
| Full stdlib            | ✅ Core (io, math, string, memory, system) |
| Pointers / Structs     | ✅ Basic        |
| Modules                | ✅ Basic (`import`) |
| GHL Studio IDE         | ✅ Web IDE (`ide/index.html`) |

The frontend is a real GHL lexer/parser/typechecker. The temporary C backend is used only for bootstrapping native output (via system `gcc`).

## Quick Start — MSYS2 UCRT64 (Windows)

Open the **UCRT64** terminal, then:

```bash
# Install toolchain (once)
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make make

# Build the compiler
cd GaphopLang/compiler
make
# → ghlc.exe

# Create & run a project
./ghlc.exe new Hello
cd Hello
../ghlc.exe build
./bin/Hello.exe
```

Full guide: [docs/installation-msys2.md](docs/installation-msys2.md)

## Quick Start — Linux

```bash
cd GaphopLang/compiler
make
./ghlc new Hello
cd Hello
../ghlc build
./bin/Hello
```

## Repository Layout

```
GaphopLang/
├── compiler/          # GHL compiler (C)
├── runtime/           # Runtime support
├── std/               # Standard library (GHL sources)
├── ide/               # GHL Studio (future)
├── docs/              # Documentation
├── examples/          # Example programs
├── tests/             # Test suite
├── tools/             # Utility scripts
└── templates/         # Project templates
```

## License

MIT License — see [LICENSE](LICENSE)

## Version

GaphopLang 0.1.0  
Compiler 0.1.0  
Targets: Linux x86-64 · Windows x64 (MSYS2 UCRT64 / MinGW)
