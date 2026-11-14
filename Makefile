
CC = clang

CFLAGS = -std=c17 \
         -Wall -Wextra -Wpedantic \
         -Wconversion -Wformat=2 \
         -g -Isrc \
         -fsanitize=address,undefined \
         -fno-omit-frame-pointer

LDFLAGS = -fsanitize=address,undefined

COMMON = src/stack.c \
         src/memory.c \
         src/opcode.c \
         src/uxn.c \
         src/execute.c

.PHONY: all test clean

all: build/uxn

build:
	mkdir -p build

build/uxn: $(COMMON) src/main.c | build
	$(CC) $(CFLAGS) $(COMMON) src/main.c -o $@ $(LDFLAGS)

build/test_uxn: $(COMMON) tests/test_main.c | build
	$(CC) $(CFLAGS) $(COMMON) tests/test_main.c -o $@

test: build/test_uxn
	./build/test_uxn

clean:
	rm -rf build
