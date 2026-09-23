CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS =

SRCS = src/common.c src/diagnostics.c src/lexer.c src/parser.c src/ast.c \
       src/typecheck.c src/codegen.c src/main.c
OBJS = $(SRCS:src/%.c=build/%.o)

.PHONY: all clean test

all: ghlc

ghlc: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

clean:
	rm -rf build ghlc

test: ghlc
	./ghlc version
