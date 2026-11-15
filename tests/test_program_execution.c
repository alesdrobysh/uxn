#include <assert.h>
#include "execute.h"

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
