#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "execute.h"

typedef struct {
    const char *name;
    uint8_t base;
    uint8_t input8[4];
    size_t input8_len;
    uint8_t output8[6];
    size_t output8_len;
    uint8_t input16[6];
    size_t input16_len;
    uint8_t output16[8];
    size_t output16_len;
} StackOpcodeCase;

static const StackOpcodeCase stack_opcode_cases[] = {
    {"POP", 0x02, {0xA1}, 1, {0}, 0, {0x12, 0x34}, 2, {0}, 0},
    {"NIP", 0x03, {0x11, 0x22}, 2, {0x22}, 1,
           {0x12, 0x34, 0x56, 0x78}, 4, {0x56, 0x78}, 2},
    {"SWP", 0x04, {0x11, 0x22}, 2, {0x22, 0x11}, 2,
           {0x12, 0x34, 0x56, 0x78}, 4, {0x56, 0x78, 0x12, 0x34}, 4},
    {"ROT", 0x05, {0x11, 0x22, 0x33}, 3, {0x22, 0x33, 0x11}, 3,
           {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}, 6,
           {0x56, 0x78, 0x9A, 0xBC, 0x12, 0x34}, 6},
    {"DUP", 0x06, {0xA1}, 1, {0xA1, 0xA1}, 2,
           {0x12, 0x34}, 2, {0x12, 0x34, 0x12, 0x34}, 4},
    {"OVR", 0x07, {0x11, 0x22}, 2, {0x11, 0x22, 0x11}, 3,
           {0x12, 0x34, 0x56, 0x78}, 4,
           {0x12, 0x34, 0x56, 0x78, 0x12, 0x34}, 6},
};

static void run_stack_opcode_case(const StackOpcodeCase *tc, bool short_mode,
                                  bool return_mode, bool keep_mode) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    UxnStack *target = return_mode ? &vm->ret : &vm->working;
    UxnStack *other = return_mode ? &vm->working : &vm->ret;
    stack_push8(target, 0xEE);
    stack_push8(other, 0xA7);

    const uint8_t *input = short_mode ? tc->input16 : tc->input8;
    const size_t input_len = short_mode ? tc->input16_len : tc->input8_len;
    const uint8_t *output = short_mode ? tc->output16 : tc->output8;
    const size_t output_len = short_mode ? tc->output16_len : tc->output8_len;
    for (size_t i = 0; i < input_len; ++i) {
        stack_push8(target, input[i]);
    }

    const uint8_t opcode = (uint8_t)(tc->base |
        (short_mode ? 0x20u : 0u) |
        (return_mode ? 0x40u : 0u) |
        (keep_mode ? 0x80u : 0u));
    vm->ram[0x0100] = opcode;
    vm->ram[0x0101] = 0x00;
    assert(uxn_run(vm, 2) == UXN_RUN_BRK);
    assert(uxn_get_pc(vm) == 0x0102);

    uint8_t expected[16] = {0xEE};
    size_t expected_len = 1;
    if (keep_mode) {
        memcpy(expected + expected_len, input, input_len);
        expected_len += input_len;
    }
    memcpy(expected + expected_len, output, output_len);
    expected_len += output_len;

    if ((size_t)target->pointer != expected_len) {
        fprintf(
            stderr, 
            "FAIL: %s\nShort mode: %d\nReturn mode: %d\nKeep mode: %d\nExpected stack pointer: %zu, actual: %d\n", 
            tc->name,
            short_mode,
            return_mode,
            keep_mode,
            expected_len, 
            target->pointer
        );
        abort();
    }

    assert(memcmp(target->data, expected, expected_len) == 0);
    assert(other->pointer == 1 && other->data[0] == 0xA7);
    uxn_destroy(vm);
}

int test_stack_opcodes(void) {
    for (size_t i = 0; i < sizeof stack_opcode_cases / sizeof stack_opcode_cases[0]; ++i) {
        for (unsigned mode = 0; mode < 8; ++mode) {
            run_stack_opcode_case(&stack_opcode_cases[i], (mode & 1u) != 0,
                                  (mode & 2u) != 0, (mode & 4u) != 0);
        }
    }
    return 0;
}

typedef struct {
    const char *name;
    uint8_t base;
    uint8_t a8;
    uint8_t b8;
    uint8_t result8;
    uint16_t a16;
    uint16_t b16;
    uint16_t result16;
} CompareCase;

static const CompareCase comparison_cases[] = {
    {"EQU 1", 0x08, 0x34, 0x34, 1, 0x1234, 0x1234, 1},
    {"EQU 0", 0x08, 0x34, 0x35, 0, 0x1234, 0x1235, 0},
    {"NEQ 1", 0x09, 0x34, 0x35, 1, 0x1234, 0x1235, 1},
    {"NEQ 0", 0x09, 0x34, 0x34, 0, 0x1234, 0x1234, 0},
    {"GTH 1", 0x0A, 0x90, 0x11, 1, 0x9000, 0x1100, 1},
    {"GTH 0", 0x0A, 0x11, 0x90, 0, 0x1100, 0x9000, 0},
    {"LTH 1", 0x0B, 0x11, 0x90, 1, 0x1100, 0x9000, 1},
    {"LTH0", 0x0B, 0x90, 0x11, 0, 0x9000, 0x1100, 0},
};

int test_comparison_opcodes(void) {
    for (size_t i = 0; i < sizeof comparison_cases / sizeof comparison_cases[0]; ++i) {
        const CompareCase *tc = &comparison_cases[i];
        for (unsigned mode = 0; mode < 8; ++mode) {
            const bool short_mode = (mode & 1u) != 0;
            const bool return_mode = (mode & 2u) != 0;
            const bool keep_mode = (mode & 4u) != 0;
            Uxn *vm = uxn_create();
            assert(vm != NULL);
            UxnStack *target = return_mode ? &vm->ret : &vm->working;
            UxnStack *other = return_mode ? &vm->working : &vm->ret;
            stack_push8(target, 0xEE);
            stack_push8(other, 0xA7);

            uint8_t inputs[4];
            size_t input_len;
            uint8_t expected_result[2];
            size_t result_len;
            if (short_mode) {
                inputs[0] = (uint8_t)(tc->a16 >> 8);
                inputs[1] = (uint8_t)tc->a16;
                inputs[2] = (uint8_t)(tc->b16 >> 8);
                inputs[3] = (uint8_t)tc->b16;
                input_len = 4;
                expected_result[0] = (uint8_t)tc->result16;
                result_len = 1;
            } else {
                inputs[0] = tc->a8;
                inputs[1] = tc->b8;
                input_len = 2;
                expected_result[0] = tc->result8;
                result_len = 1;
            }
            for (size_t j = 0; j < input_len; ++j) {
                stack_push8(target, inputs[j]);
            }

            vm->ram[0x0100] = (uint8_t)(tc->base |
                (short_mode ? 0x20u : 0u) |
                (return_mode ? 0x40u : 0u) |
                (keep_mode ? 0x80u : 0u));
            vm->ram[0x0101] = 0;
            assert(uxn_run(vm, 2) == UXN_RUN_BRK);

            uint8_t expected[8] = {0xEE};
            size_t expected_len = 1;
            if (keep_mode) {
                memcpy(expected + expected_len, inputs, input_len);
                expected_len += input_len;
            }
            memcpy(expected + expected_len, expected_result, result_len);
            expected_len += result_len;
            if ((size_t)target->pointer != expected_len) {
                fprintf(
                    stderr, 
                    "FAIL: %s\nShort mode: %d\nReturn mode: %d\nKeep mode: %d\nExpected stack pointer: %zu, actual: %d\n", 
                    tc->name,
                    short_mode,
                    return_mode,
                    keep_mode,
                    expected_len, 
                    target->pointer
                );
                abort();
            }
            assert(memcmp(target->data, expected, expected_len) == 0);
            assert(other->pointer == 1 && other->data[0] == 0xA7);
            uxn_destroy(vm);
        }
    }
    return 0;
}

int test_control_opcodes(void) {
    for (unsigned mode = 0; mode < 8; ++mode) {
        const bool short_mode = (mode & 1u) != 0;
        const bool return_mode = (mode & 2u) != 0;
        const bool keep_mode = (mode & 4u) != 0;

        const uint8_t mode_bits = (uint8_t)(
            (short_mode ? 0x20u : 0u) |
            (return_mode ? 0x40u : 0u) |
            (keep_mode ? 0x80u : 0u)
        );

        /*
         * JMP:
         * - 8-bit target: signed relative offset
         * - 16-bit target: absolute address
         */
        Uxn *vm = uxn_create();
        assert(vm != NULL);

        UxnStack *source = return_mode
            ? &vm->ret
            : &vm->working;

        UxnStack *other = return_mode
            ? &vm->working
            : &vm->ret;

        stack_push8(source, 0xEE);
        stack_push8(other, 0xA7);

        if (short_mode) {
            stack_push16(source, 0x2345);
        } else {
            stack_push8(source, 2);
        }

        vm->ram[0x0100] = (uint8_t)(0x0Cu | mode_bits);

        assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);

        assert(uxn_get_pc(vm) ==
            (short_mode ? 0x2345 : 0x0103));

        assert(source->pointer == (uint8_t)(
            1u + (keep_mode ? (short_mode ? 2u : 1u) : 0u)
        ));

        assert(source->data[0] == 0xEE);

        if (keep_mode) {
            if (short_mode) {
                assert(source->data[1] == 0x23);
                assert(source->data[2] == 0x45);
            } else {
                assert(source->data[1] == 2);
            }
        }

        assert(other->pointer == 1);
        assert(other->data[0] == 0xA7);

        uxn_destroy(vm);

        /*
         * JCN:
         * - condition is always 8-bit
         * - target is 8-bit or 16-bit
         * - keep preserves both operands
         */
        vm = uxn_create();
        assert(vm != NULL);

        source = return_mode ? &vm->ret : &vm->working;
        other = return_mode ? &vm->working : &vm->ret;

        stack_push8(source, 0xEE);
        stack_push8(other, 0xA7);

        stack_push8(source, 1); // True condition

        if (short_mode) {
            stack_push16(source, 0x2345);
        } else {
            stack_push8(source, 2);
        }

        vm->ram[0x0100] = (uint8_t)(0x0Du | mode_bits);

        assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);

        assert(uxn_get_pc(vm) ==
            (short_mode ? 0x2345 : 0x0103));

        assert(source->pointer == (uint8_t)(
            1u + (keep_mode ? (short_mode ? 3u : 2u) : 0u)
        ));

        assert(source->data[0] == 0xEE);

        if (keep_mode) {
            assert(source->data[1] == 1);

            if (short_mode) {
                assert(source->data[2] == 0x23);
                assert(source->data[3] == 0x45);
            } else {
                assert(source->data[2] == 2);
            }
        }

        assert(other->pointer == 1);
        assert(other->data[0] == 0xA7);

        uxn_destroy(vm);

        /*
         * JSR:
         * - target comes from source
         * - return address goes to the OPPOSITE stack
         * - return address is always 16-bit
         * - keep preserves target in source
         */
        vm = uxn_create();
        assert(vm != NULL);

        source = return_mode ? &vm->ret : &vm->working;
        other = return_mode ? &vm->working : &vm->ret;

        stack_push8(source, 0xEE);

        if (short_mode) {
            stack_push16(source, 0x2345);
        } else {
            stack_push8(source, 2);
        }

        vm->ram[0x0100] = (uint8_t)(0x0Eu | mode_bits);

        assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);

        assert(uxn_get_pc(vm) ==
            (short_mode ? 0x2345 : 0x0103));

        const size_t target_len = short_mode ? 2u : 1u;

        /* Source stack: target consumed or preserved */
        assert(source->pointer == (uint8_t)(
            1u + (keep_mode ? target_len : 0u)
        ));

        assert(source->data[0] == 0xEE);

        if (keep_mode) {
            if (short_mode) {
                assert(source->data[1] == 0x23);
                assert(source->data[2] == 0x45);
            } else {
                assert(source->data[1] == 2);
            }
        }

        /*
         * Destination stack: return address = 0x0101.
         * This works for JSR and JSRr alike.
         */
        assert(other->pointer == 2);
        assert(other->data[0] == 0x01);
        assert(other->data[1] == 0x01);

        uxn_destroy(vm);
    }

    /*
     * STH:
     * - r selects the source stack
     * - short selects the value width
     * - keep preserves the source operand
     */
    for (unsigned mode = 0; mode < 8; ++mode) {
        const bool short_mode = (mode & 1u) != 0;
        const bool return_mode = (mode & 2u) != 0;
        const bool keep_mode = (mode & 4u) != 0;

        Uxn *vm = uxn_create();
        assert(vm != NULL);

        UxnStack *source = return_mode
            ? &vm->ret
            : &vm->working;

        UxnStack *destination = return_mode
            ? &vm->working
            : &vm->ret;

        stack_push8(source, 0xEE);

        if (short_mode) {
            stack_push16(source, 0x1234);
        } else {
            stack_push8(source, 0xA5);
        }

        vm->ram[0x0100] = (uint8_t)(
            0x0Fu |
            (short_mode ? 0x20u : 0u) |
            (return_mode ? 0x40u : 0u) |
            (keep_mode ? 0x80u : 0u)
        );

        assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);
        assert(uxn_get_pc(vm) == 0x0101);

        const size_t value_len = short_mode ? 2u : 1u;

        assert(source->pointer == (uint8_t)(
            1u + (keep_mode ? value_len : 0u)
        ));

        assert(destination->pointer == value_len);

        if (short_mode) {
            assert(destination->data[0] == 0x12);
            assert(destination->data[1] == 0x34);
        } else {
            assert(destination->data[0] == 0xA5);
        }

        assert(source->data[0] == 0xEE);

        if (keep_mode) {
            if (short_mode) {
                assert(source->data[1] == 0x12);
                assert(source->data[2] == 0x34);
            } else {
                assert(source->data[1] == 0xA5);
            }
        }

        uxn_destroy(vm);
    }

    return 0;
}

int test_immediate_control_and_literal_opcodes(void) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    const uint8_t literals[] = {
        0x80, 0x12,       /* LIT */
        0xA0, 0x34, 0x56, /* LIT2 */
        0xC0, 0x78,       /* LITR */
        0xE0, 0x9A, 0xBC, /* LIT2R */
        0x00
    };
    copy_bytes(&vm->ram[0x0100], literals, sizeof literals);
    assert(uxn_run(vm, 5) == UXN_RUN_BRK);
    assert(uxn_get_pc(vm) == (uint16_t)(0x0100 + sizeof literals));
    assert(vm->working.pointer == 3);
    assert(vm->working.data[0] == 0x12);
    assert(vm->working.data[1] == 0x34 && vm->working.data[2] == 0x56);
    assert(vm->ret.pointer == 3);
    assert(vm->ret.data[0] == 0x78);
    assert(vm->ret.data[1] == 0x9A && vm->ret.data[2] == 0xBC);

    uxn_init(vm);
    /* JMI's unsigned 16-bit addition also represents backward two's-complement offsets. */
    vm->ram[0x0100] = 0x40;
    vm->ram[0x0101] = 0xFF;
    vm->ram[0x0102] = 0xFD;
    assert(uxn_run(vm, 2) == UXN_RUN_STEP_LIMIT);
    assert(uxn_get_pc(vm) == 0x0100);

    uxn_init(vm);
    vm->ram[0x0100] = 0x20; /* JCI */
    vm->ram[0x0101] = 0x00;
    vm->ram[0x0102] = 0x02;
    vm->ram[0x0103] = 0x80;
    vm->ram[0x0104] = 0x11;
    vm->ram[0x0105] = 0x80;
    vm->ram[0x0106] = 0x22;
    vm->ram[0x0107] = 0x00;
    stack_push8(&vm->working, 1);
    assert(uxn_run(vm, 3) == UXN_RUN_BRK);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x22);

    uxn_init(vm);
    vm->ram[0x0100] = 0x60; /* JSI */
    vm->ram[0x0101] = 0x00;
    vm->ram[0x0102] = 0x02;
    vm->ram[0x0103] = 0x80;
    vm->ram[0x0104] = 0x11;
    vm->ram[0x0105] = 0x80;
    vm->ram[0x0106] = 0x2A;
    vm->ram[0x0107] = 0x00;
    assert(uxn_run(vm, 3) == UXN_RUN_BRK);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x2A);
    assert(vm->ret.pointer == 2);
    assert(vm->ret.data[0] == 0x01 && vm->ret.data[1] == 0x03);

    uxn_destroy(vm);
    return 0;
}

int test_decode_opcode_exhaustive(void) {
    static const char *const names[32] = {
        NULL, "INC", "POP", "NIP", "SWP", "ROT", "DUP", "OVR",
        "EQU", "NEQ", "GTH", "LTH", "JMP", "JCN", "JSR", "STH",
        "LDZ", "STZ", "LDR", "STR", "LDA", "STA", "DEI", "DEO",
        "ADD", "SUB", "MUL", "DIV", "AND", "ORA", "EOR", "SFT"
    };
    for (unsigned raw = 0; raw <= 0xFF; ++raw) {
        const DecodedOpcode op = decode_opcode((uint8_t)raw);
        if ((raw & 0x1Fu) == 0 &&
            (raw == 0x00 || raw == 0x20 || raw == 0x40 || raw == 0x60 ||
             raw == 0x80 || raw == 0xA0 || raw == 0xC0 || raw == 0xE0)) {
            const OpcodeKind expected[] = {
                OP_BRK, OP_JCI, OP_JMI, OP_JSI,
                OP_LIT, OP_LIT2, OP_LITR, OP_LIT2R
            };
            const unsigned index = raw >> 5;
            assert(op.kind == expected[index]);
        } else {
            assert(op.kind == OP_STANDARD);
            assert(op.base == (uint8_t)(raw & 0x1Fu));
            assert(op.short_mode == ((raw & 0x20u) != 0));
            assert(op.return_mode == ((raw & 0x40u) != 0));
            assert(op.keep_mode == ((raw & 0x80u) != 0));
        }
    }

    for (unsigned base = 1; base <= 0x1F; ++base) {
        assert(strcmp(opcode_name((uint8_t)base), names[base]) == 0);
    }
    assert(opcode_name(0) == NULL);
    assert(opcode_name(0x20) == NULL);
    assert(strcmp(format_opcode_kind(OP_STANDARD), "OP_STANDARD") == 0);
    assert(strcmp(format_opcode_kind(OP_BRK), "OP_BRK") == 0);
    assert(strcmp(format_opcode_kind(OP_JCI), "OP_JCI") == 0);
    assert(strcmp(format_opcode_kind(OP_JMI), "OP_JMI") == 0);
    assert(strcmp(format_opcode_kind(OP_JSI), "OP_JSI") == 0);
    assert(strcmp(format_opcode_kind(OP_LIT), "OP_LIT") == 0);
    assert(strcmp(format_opcode_kind(OP_LIT2), "OP_LIT2") == 0);
    assert(strcmp(format_opcode_kind(OP_LITR), "OP_LITR") == 0);
    assert(strcmp(format_opcode_kind(OP_LIT2R), "OP_LIT2R") == 0);
    return 0;
}
