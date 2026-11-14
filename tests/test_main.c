#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "execute.h"

void debug_array(const uint8_t* arr, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%0X " , arr[i]);
    }

    printf("\n");
}

int test_vm_structure(void) {
    Uxn *vm = uxn_create();

    if (vm == NULL) {
        return 1;
    }

    assert(vm->pc == 0x100);

    assert(vm->working.pointer == 0);
    assert(vm->ret.pointer == 0);

    assert(vm->working.data[0] == 0);
    assert(vm->working.data[128] == 0);
    assert(vm->working.data[255] == 0);

    assert(vm->ret.data[0] == 0);
    assert(vm->ret.data[128] == 0);
    assert(vm->ret.data[255] == 0);

    assert(vm->dev[0] == 0);
    assert(vm->dev[128] == 0);
    assert(vm->dev[255] == 0);

    assert(vm->ram[0] == 0);
    assert(vm->ram[32255] == 0);
    assert(vm->ram[65535] == 0);

    uxn_set_pc(vm, 0x1234);

    assert(uxn_get_pc(vm) == 0x1234);

    stack_push8(&vm->ret, 0x2A);


    assert(vm->ret.data[0] == 0x2A);
    assert(vm->ret.pointer == 1);
    assert(stack_pop8(&vm->ret) == 0x2A);
    assert(vm->ret.pointer == 0);

    stack_push8(&vm->ret, 0x12);
    stack_push8(&vm->ret, 0x34);

    assert(stack_pop8(&vm->ret) == 0x34);
    assert(stack_pop8(&vm->ret) == 0x12);
    
    stack_push16(&vm->ret, 0xABCD);

    assert(vm->ret.data[0] == 0xAB);
    assert(vm->ret.data[1] == 0xCD);
    assert(vm->ret.pointer == 2);
    assert(stack_pop16(&vm->ret) == 0xABCD);

    vm->ret.pointer = 255;

    stack_push8(&vm->ret, 0x2A);

    assert(vm->ret.data[255] == 0x2A);
    assert(vm->ret.pointer == 0);
    assert(stack_pop8(&vm->ret) == 0x2A);
    assert(vm->ret.pointer == 255);

    stack_push16(&vm->ret, 0xABCD);

    assert(vm->ret.data[255] == 0xAB);
    assert(vm->ret.data[0] == 0xCD);
    assert(vm->ret.pointer == 1);
    assert(stack_pop16(&vm->ret) == 0xABCD);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}

int test_run_lit_lit2(void) {
    Uxn *vm = uxn_create();

    uint8_t program[] = {0x80, 0x2A, 0xA0, 0x12, 0x34, 0x00};
    copy_bytes(&vm->ram[0x0100], program, 6);

    uxn_run(vm, 5);

    assert(vm->working.data[0] == 0x2A);
    assert(vm->working.data[1] == 0x12);
    assert(vm->working.data[2] == 0x34);
    assert(vm->working.pointer == 3);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}

int test_run_jmi(void) {
    // forward

    Uxn *vm = uxn_create();

    uint8_t program[] = {0x40, 0x00, 0x03, 0x80, 0x11, 0x00, 0x80, 0x22, 0x00};
    copy_bytes(&vm->ram[0x0100], program, 9);
    
    uxn_run(vm, 10);

    assert(vm->working.data[0] == 0x22);
    assert(vm->working.pointer == 1);
    assert(vm->pc == 0x0109);

    uxn_destroy(vm);
    vm = NULL;

    // backwards

    vm = uxn_create();

    uint8_t program2[] = {0x40, 0xFF, 0xFD};
    copy_bytes(&vm->ram[0x0100], program2, 3);
    
    UxnRunResult result = uxn_run(vm, 4);

    assert(result == UXN_RUN_STEP_LIMIT);
    assert(vm->pc == 0x0100);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}

int test_run_jci(void) {
    // condition is 1
    
    Uxn *vm = uxn_create();

    uint8_t program[] = {0x80, 0x01, 0x20, 0x00, 0x03, 0x80, 0x11, 0x00, 0x80, 0x22, 0x00};
    copy_bytes(&vm->ram[0x0100], program, 11);
    
    uxn_run(vm, 11);

    assert(vm->working.data[0] == 0x22);
    assert(vm->working.pointer == 1);
    assert(vm->pc == 0x10B);

    uxn_destroy(vm);
    vm = NULL;

    // condition is 0

    vm = uxn_create();

    program[1] = 0x00;
    copy_bytes(&vm->ram[0x0100], program, 11);
    
    uxn_run(vm, 11);

    assert(vm->working.data[0] == 0x11);
    assert(vm->working.pointer == 1);
    assert(vm->pc == 0x108);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}

int test_run_jsi(void) {
    Uxn *vm = uxn_create();

    uint8_t program[] = {0x60, 0x00, 0x03, 0x80, 0x11, 0x00, 0x80, 0x2A, 0x00};
    copy_bytes(&vm->ram[0x0100], program, 9);
    
    uxn_run(vm, 9);

    assert(vm->working.data[0] == 0x2A);
    assert(vm->ret.data[0] == 0x01);
    assert(vm->ret.data[1] == 0x03);
    assert(vm->pc == 0x0109);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}

int test_decode_opcode(void) {
    DecodedOpcode op;

    op = decode_opcode(0x01);
    assert(op.kind == OP_STANDARD);
    assert(op.base == 0x01);
    assert(!op.short_mode);
    assert(!op.return_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x21);
    assert(op.base == 0x01);
    assert(op.short_mode);
    assert(!op.return_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x41);
    assert(op.return_mode);
    assert(!op.short_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x81);
    assert(op.keep_mode);
    assert(!op.short_mode);
    assert(!op.return_mode);

    op = decode_opcode(0xF8);
    assert(op.kind == OP_STANDARD);
    assert(op.base == 0x18);
    assert(op.short_mode);
    assert(op.return_mode);
    assert(op.keep_mode);

    assert(decode_opcode(0x00).kind == OP_BRK);
    assert(decode_opcode(0x20).kind == OP_JCI);
    assert(decode_opcode(0x40).kind == OP_JMI);
    assert(decode_opcode(0x60).kind == OP_JSI);
    assert(decode_opcode(0x80).kind == OP_LIT);
    assert(decode_opcode(0xA0).kind == OP_LIT2);
    assert(decode_opcode(0xC0).kind == OP_LITR);
    assert(decode_opcode(0xE0).kind == OP_LIT2R);

    assert(strcmp(opcode_name(0x01), "INC") == 0);
    assert(strcmp(opcode_name(0x18), "ADD") == 0);
    assert(strcmp(opcode_name(0x1F), "SFT") == 0);

    assert(opcode_name(0x00) == NULL);
    assert(opcode_name(0x20) == NULL);

    return 0;
}

int main(void) {
    int result = test_vm_structure() || 
        test_run_lit_lit2() || 
        test_run_jmi() ||
        test_run_jci() ||
        test_run_jsi() ||
        test_decode_opcode();

    puts("All tests passed!");

    return result;
}
