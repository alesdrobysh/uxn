
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

TESTS = tests/test_main.c \
        tests/test_vm.c \
        tests/test_program_execution.c \
        tests/test_arithmetic.c \
        tests/test_opcode.c \
        tests/test_memory.c \
        tests/test_vm_opcodes.c

.PHONY: all test clean

all: build/uxn

build:
	mkdir -p build

build/uxn: $(COMMON) src/main.c | build
	$(CC) $(CFLAGS) $(COMMON) src/main.c -o $@ $(LDFLAGS)

build/test_uxn: $(COMMON) $(TESTS) tests/test_cases.h | build
	$(CC) $(CFLAGS) $(COMMON) $(TESTS) -o $@

test: build/test_uxn
	./build/test_uxn

clean:
	rm -rf build
