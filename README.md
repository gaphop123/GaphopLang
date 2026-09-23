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

## Current Status: Phase 1 (Bootstrap)

| Component              | Status          |
|------------------------|-----------------|
| Lexer                  | ✅ Implemented  |
| Parser + AST           | ✅ Implemented  |
| Basic Type Checker     | ✅ Implemented  |
| Codegen (C backend)    | ✅ Implemented  |
| `ghlc build` / `run`   | ✅ Working      |
| Hello World            | ✅ Working      |
| Project system         | 🚧 Basic        |
| Full stdlib            | 🚧 Minimal      |
| Pointers / Structs     | ⏳ Phase 2      |
| Modules                | ⏳ Phase 2      |
| GHL Studio IDE         | ⏳ Phase 4      |
| Windows .exe native    | ⏳ Later (Linux ELF first) |

**Note:** Phase 1 targets Linux x86-64. Windows support (via MinGW/LLVM) is planned. The frontend is a real GHL lexer/parser/typechecker; the temporary C backend is only for bootstrapping native output.

## Quick Start

```bash
# Build the compiler
cd compiler
make

# Create a project
./ghlc new Hello
cd Hello

# Build & run
../compiler/ghlc build
../compiler/ghlc run
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
Target: Linux x86-64 (Windows planned)
