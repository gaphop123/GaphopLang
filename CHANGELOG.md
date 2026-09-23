# Changelog

## [0.1.0] - 2026-09-23 — Phase 1 Bootstrap

### Added
- Full GHL lexer (keywords, operators, comments, strings, numbers)
- Recursive-descent parser producing typed AST
- Strict type checker with diagnostics (GHL1xx / GHL2xx codes)
- C backend code generator
- CLI: `ghlc new`, `build`, `run`, `check`, `clean`, `version`, `help`
- Project scaffold via `ghlc new`
- Basic standard print helpers: `print`, `println`, `print_int`, `print_float`
- Error reporting with location, code, and help text

### Notes
- Targets Linux x86-64 via gcc backend
- Windows .exe and full IR/LLVM backend planned for later phases
