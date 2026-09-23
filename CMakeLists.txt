cmake_minimum_required(VERSION 3.20)

project(GaphopLang
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(GaphopLang
    compiler/src/main.c
    compiler/src/ast.c
    compiler/src/codegen.c
    compiler/src/common.c
    compiler/src/diagnostics.c
    compiler/src/lexer.c
    compiler/src/parser.c
    compiler/src/typecheck.c
)

target_include_directories(GaphopLang PRIVATE
    compiler/include
)
