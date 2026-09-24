# GaphopLang Compiler Makefile
# Works on: Linux, MSYS2 (UCRT64 / MINGW64), MinGW

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS ?=

# Detect Windows (MSYS2 / MinGW)
ifeq ($(OS),Windows_NT)
    EXEEXT := .exe
    RM     := rm -f
    MKDIR  := mkdir -p
else
    # Also detect MSYS2 via uname
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq (,$(findstring MINGW,$(UNAME_S)))
        EXEEXT := .exe
    else ifneq (,$(findstring MSYS,$(UNAME_S)))
        EXEEXT := .exe
    else
        EXEEXT :=
    endif
    RM    := rm -f
    MKDIR := mkdir -p
endif

TARGET  := ghlc$(EXEEXT)

SRCS = src/common.c src/diagnostics.c src/lexer.c src/parser.c src/ast.c \
       src/typecheck.c src/codegen.c src/main.c
OBJS = $(SRCS:src/%.c=build/%.o)

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo ""
	@echo "Built: $(TARGET)"
	@echo "Run:   ./$(TARGET) version"

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	$(MKDIR) build

clean:
	$(RM) -r build $(TARGET) ghlc ghlc.exe

test: $(TARGET)
	./$(TARGET) version

# Optional: install into MSYS2 UCRT64 prefix
PREFIX ?= /ucrt64
install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/
	@echo "Installed to $(PREFIX)/bin/$(TARGET)"
