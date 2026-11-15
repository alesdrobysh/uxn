#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "execute.h"

typedef struct {
    const char *name;
    uint8_t opcode;
    bool return_mode;
    uint8_t before[4];
    size_t before_len;
    uint8_t after[6];
    size_t after_len;
} UxnInstructionCase;

typedef struct {
    const char *name;
    uint8_t base;
    uint8_t in8[2];
    uint8_t out8;
    uint8_t in16[4];
    size_t in16_len; /* SFT2 consumes 3 bytes; others consume 4. */
    uint8_t out16[2];
} ArithmeticCase;

static const ArithmeticCase arithmetic_cases[] = {
    {"SUB", 0x19, {0x29, 0x0A}, 0x1F,
        {0x12, 0x34, 0x01, 0x02}, 4, {0x11, 0x32}},
    {"MUL", 0x1A, {0x13, 0x09}, 0xAB,
        {0x12, 0x34, 0x00, 0x03}, 4, {0x36, 0x9C}},
    {"DIV", 0x1B, {0x29, 0x0A}, 0x04,
        {0x12, 0x34, 0x00, 0x03}, 4, {0x06, 0x11}},
    {"AND", 0x1C, {0xAC, 0x3C}, 0x2C,
        {0x12, 0x34, 0x0F, 0x0F}, 4, {0x02, 0x04}},
    {"ORA", 0x1D, {0xAC, 0x3C}, 0xBC,
        {0x12, 0x34, 0x0F, 0x0F}, 4, {0x1F, 0x3F}},
    {"EOR", 0x1E, {0xAC, 0x3C}, 0x90,
        {0x12, 0x34, 0x0F, 0x0F}, 4, {0x1D, 0x3B}},
    {"SFT", 0x1F, {0x34, 0x10}, 0x68,
        {0x12, 0x48, 0x34, 0x00}, 3, {0x09, 0x20}},
};

typedef struct {
    const char *name;
    uint8_t opcode;
    uint8_t input[4];
    size_t input_len;
    uint8_t output[2];
    size_t output_len;
} EdgeCase;

static const EdgeCase edge_cases[] = {
    {"SUB 8-bit underflow", 0x19, {0x02, 0x09}, 2, {0xF9}, 1},
    {"SUB2 underflow", 0x39, {0x00, 0x02, 0x00, 0x09}, 4, {0xFF, 0xF9}, 2},
    {"MUL 8-bit wrap", 0x1A, {0xFF, 0x02}, 2, {0xFE}, 1},
    {"MUL2 16-bit wrap", 0x3A, {0xFF, 0xFF, 0xFF, 0xFF}, 4, {0x00, 0x01}, 2},
    {"DIV by zero", 0x1B, {0x09, 0x00}, 2, {0x00}, 1},
    {"DIV2 by zero", 0x3B, {0x12, 0x34, 0x00, 0x00}, 4, {0x00, 0x00}, 2},
    {"DIV2 integer truncation", 0x3B, {0xFF, 0xFF, 0x00, 0xFF}, 4, {0x01, 0x01}, 2},
    {"SFT 8-bit right", 0x1F, {0x34, 0x01}, 2, {0x1A}, 1},
    {"SFT right before left", 0x1F, {0xFF, 0x14}, 2, {0x1E}, 1},
    {"SFT2 right 15", 0x3F, {0x80, 0x00, 0x0F}, 3, {0x00, 0x01}, 2},
    {"SFT2 left 15", 0x3F, {0x00, 0x01, 0xF0}, 3, {0x80, 0x00}, 2},
    {"SFT2k preserves 3 bytes", 0xBF, {0x12, 0x48, 0x34}, 3, {0x09, 0x20}, 2},
    {"SFT2rk preserves 3 bytes", 0xFF, {0x12, 0x48, 0x34}, 3, {0x09, 0x20}, 2},
    {"DIVk by zero keeps operands", 0x9B, {0x09, 0x00}, 2, {0x00}, 1},
};

static void run_one(const char *name, uint8_t opcode,
                    const uint8_t *input, size_t input_len,
                    const uint8_t *output, size_t output_len) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    const bool return_mode = (opcode & 0x40u) != 0;
    const bool keep_mode = (opcode & 0x80u) != 0;
    UxnStack *target = return_mode ? &vm->ret : &vm->working;
    UxnStack *other = return_mode ? &vm->working : &vm->ret;

    /* A prefix ensures the opcode doesn't destroy unrelated stack items. */
    stack_push8(target, 0x7E);
    stack_push8(other, 0xA7);
    for (size_t i = 0; i < input_len; ++i) {
        stack_push8(target, input[i]);
    }

    vm->ram[0x0100] = opcode;
    vm->ram[0x0101] = 0x00; /* BRK */
    const UxnRunResult result = uxn_run(vm, 2);

    /* Expected: prefix + (original operands, only if k) + result. */
    uint8_t expected[8] = {0x7E};
    size_t expected_len = 1;
    if (keep_mode) {
        memcpy(expected + expected_len, input, input_len);
        expected_len += input_len;
    }
    memcpy(expected + expected_len, output, output_len);
    expected_len += output_len;

    if (result != UXN_RUN_BRK || vm->pc != 0x0102 ||
        (size_t)target->pointer != expected_len ||
        memcmp(target->data, expected, expected_len) != 0 ||
        other->pointer != 1 || other->data[0] != 0xA7) {
        fprintf(stderr,
            "FAIL %-29s opcode=%02X result=%d pc=%04X target_ptr=%u other_ptr=%u\n",
            name, (unsigned)opcode, (int)result, (unsigned)vm->pc,
            (unsigned)target->pointer, (unsigned)other->pointer);
        fprintf(stderr, "  expected:");
        for (size_t i = 0; i < expected_len; ++i)
            fprintf(stderr, " %02X", (unsigned)expected[i]);
        fprintf(stderr, "\n  actual:  ");
        for (size_t i = 0; i < expected_len; ++i)
            fprintf(stderr, " %02X", (unsigned)target->data[i]);
        fprintf(stderr, "\n");
        abort();
    }

    uxn_destroy(vm);
}

int test_arithmetic(void) {
    static const UxnInstructionCase inc_add_cases[] = {
        /* INC: normal, 2, r, 2r, k, 2k, rk, 2rk */
        {"INC",     0x01, false, {0x2A},                 1, {0x2B},                         1},
        {"INC2",    0x21, false, {0x12, 0xFF},           2, {0x13, 0x00},                   2},
        {"INCr",    0x41, true,  {0x2A},                 1, {0x2B},                         1},
        {"INC2r",   0x61, true,  {0x12, 0xFF},           2, {0x13, 0x00},                   2},
        {"INCk",    0x81, false, {0x2A},                 1, {0x2A, 0x2B},                   2},
        {"INC2k",   0xA1, false, {0x12, 0xFF},           2, {0x12, 0xFF, 0x13, 0x00},       4},
        {"INCrk",   0xC1, true,  {0x2A},                 1, {0x2A, 0x2B},                   2},
        {"INC2rk",  0xE1, true,  {0x12, 0xFF},           2, {0x12, 0xFF, 0x13, 0x00},       4},
        /* ADD: normal, 2, r, 2r, k, 2k, rk, 2rk */
        {"ADD",     0x18, false, {0x07, 0x05},           2, {0x0C},                         1},
        {"ADD2",    0x38, false, {0x12, 0x34, 0x00, 0x02}, 4, {0x12, 0x36},                 2},
        {"ADDr",    0x58, true,  {0x07, 0x05},           2, {0x0C},                         1},
        {"ADD2r",   0x78, true,  {0x12, 0x34, 0x00, 0x02}, 4, {0x12, 0x36},                 2},
        {"ADDk",    0x98, false, {0x07, 0x05},           2, {0x07, 0x05, 0x0C},             3},
        {"ADD2k",   0xB8, false, {0x12, 0x34, 0x00, 0x02}, 4, {0x12, 0x34, 0x00, 0x02, 0x12, 0x36}, 6},
        {"ADDrk",   0xD8, true,  {0x07, 0x05},           2, {0x07, 0x05, 0x0C},             3},
        {"ADD2rk",  0xF8, true,  {0x12, 0x34, 0x00, 0x02}, 4, {0x12, 0x34, 0x00, 0x02, 0x12, 0x36}, 6},
        /* Explicit 8/16-bit overflow tests */
        {"INC overflow",  0x01, false, {0xFF},                1, {0x00}, 1},
        {"INC2 overflow", 0x21, false, {0xFF, 0xFF},          2, {0x00, 0x00}, 2},
        {"ADD overflow",  0x18, false, {0xFF, 0x01},          2, {0x00}, 1},
        {"ADD2 overflow", 0x38, false, {0xFF, 0xFF, 0x00, 0x01}, 4, {0x00, 0x00}, 2},
    };
    for (size_t i = 0; i < sizeof inc_add_cases / sizeof inc_add_cases[0]; ++i) {
        const UxnInstructionCase *tc = &inc_add_cases[i];
        Uxn *vm = uxn_create();
        assert(vm != NULL);

        UxnStack *target = tc->return_mode ? &vm->ret : &vm->working;
        UxnStack *other = tc->return_mode ? &vm->working : &vm->ret;
        stack_push8(other, 0x99);
        for (size_t j = 0; j < tc->before_len; ++j) {
            stack_push8(target, tc->before[j]);
        }

        vm->ram[0x0100] = tc->opcode;
        vm->ram[0x0101] = 0x00; /* BRK */
        const UxnRunResult result = uxn_run(vm, 2);

        if (result != UXN_RUN_BRK || vm->pc != 0x0102 ||
            (size_t)target->pointer != tc->after_len ||
            other->pointer != 1 || other->data[0] != 0x99 ||
            memcmp(target->data, tc->after, tc->after_len) != 0) {
            fprintf(stderr, "FAIL %s: result=%d, pc=0x%04X, stack pointer=%u\n",
                    tc->name, (int)result,
                    (unsigned)vm->pc, (unsigned)target->pointer);
            abort();
        }

        uxn_destroy(vm);
    }

    /* 7 instructions x 8 combinations of 2/r/k = 56 tests. */
    for (size_t i = 0; i < sizeof arithmetic_cases / sizeof arithmetic_cases[0]; ++i) {
        const ArithmeticCase *tc = &arithmetic_cases[i];
        for (unsigned mode = 0; mode < 8; ++mode) {
            const bool is_short = (mode & 1u) != 0;
            const uint8_t opcode = (uint8_t)(tc->base |
                (is_short ? 0x20u : 0u) |
                ((mode & 2u) ? 0x40u : 0u) |
                ((mode & 4u) ? 0x80u : 0u));
            const uint8_t *input = is_short ? tc->in16 : tc->in8;
            const size_t input_len = is_short ? tc->in16_len : 2;
            const uint8_t *output = is_short ? tc->out16 : &tc->out8;
            const size_t output_len = is_short ? 2 : 1;
            run_one(tc->name, opcode, input, input_len, output, output_len);
        }
    }

    /* Edge cases: wraparound, zero division, mixed-width SFT. */
    for (size_t i = 0; i < sizeof edge_cases / sizeof edge_cases[0]; ++i) {
        const EdgeCase *tc = &edge_cases[i];
        run_one(tc->name, tc->opcode, tc->input, tc->input_len,
                tc->output, tc->output_len);
    }

    return 0;
}
