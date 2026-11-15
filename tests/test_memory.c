#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "execute.h"

int test_memory_helpers(void) {
    uint8_t memory[UXN_RAM_SIZE] = {0};
    uint8_t source[] = {0x10, 0x20, 0x30, 0x40};
    uint8_t copy[sizeof source] = {0};

    write8(memory, 0x1234, 0xA5);
    assert(read8(memory, 0x1234) == 0xA5);

    write16(memory, 0x1234, 0xBEEF);
    assert(memory[0x1234] == 0xBE);
    assert(memory[0x1235] == 0xEF);
    assert(read16(memory, 0x1234) == 0xBEEF);

    /* A 16-bit access at the end of RAM wraps its second byte to address 0. */
    write16(memory, 0xFFFF, 0xCAFE);
    assert(memory[0xFFFF] == 0xCA);
    assert(memory[0x0000] == 0xFE);
    assert(read16(memory, 0xFFFF) == 0xCAFE);

    fill_bytes(copy, sizeof copy, 0x7B);
    for (size_t i = 0; i < sizeof copy; ++i) {
        assert(copy[i] == 0x7B);
    }
    copy_bytes(copy, source, sizeof source);
    assert(equal_bytes(copy, source, sizeof source));
    assert(!equal_bytes(copy, memory, sizeof copy));
    assert(equal_bytes(copy, source, 0));

    return 0;
}

int test_vm_initialization_and_reset(void) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);
    assert(uxn_get_pc(vm) == 0x0100);

    vm->ram[0] = 0xAA;
    vm->ram[UXN_RAM_SIZE - 1] = 0xBB;
    vm->dev[0] = 0xCC;
    vm->dev[UXN_DEV_SIZE - 1] = 0xDD;
    stack_push8(&vm->working, 0x11);
    stack_push16(&vm->ret, 0x2233);
    uxn_set_pc(vm, 0x4567);

    uxn_init(vm);
    assert(uxn_get_pc(vm) == 0x0100);
    assert(vm->working.pointer == 0);
    assert(vm->ret.pointer == 0);
    for (size_t i = 0; i < UXN_RAM_SIZE; ++i) {
        assert(vm->ram[i] == 0);
    }
    for (size_t i = 0; i < UXN_DEV_SIZE; ++i) {
        assert(vm->dev[i] == 0);
    }
    for (size_t i = 0; i < UXN_STACK_SIZE; ++i) {
        assert(vm->working.data[i] == 0);
        assert(vm->ret.data[i] == 0);
    }

    uxn_destroy(vm);
    return 0;
}

static void run_single_opcode(Uxn *vm, uint8_t opcode) {
    vm->ram[0x0100] = opcode;
    vm->ram[0x0101] = 0x00;
    assert(uxn_run(vm, 2) == UXN_RUN_BRK);
    assert(uxn_get_pc(vm) == 0x0102);
}

int test_memory_device_opcodes(void) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    /* LDZ / LDZ2 read zero-page addresses; 16-bit values are big-endian. */
    vm->ram[0x0034] = 0xA5;
    stack_push8(&vm->working, 0x34);
    run_single_opcode(vm, 0x10);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0xA5);

    uxn_init(vm);
    vm->ram[0x00FF] = 0x12;
    vm->ram[0x0100] = 0x34;
    stack_push8(&vm->working, 0xFF);
    run_single_opcode(vm, 0x30);
    assert(vm->working.pointer == 2);
    assert(vm->working.data[0] == 0x12 && vm->working.data[1] == 0x34);

    /* STZ and STZ2 take the address at the top of the stack. */
    uxn_init(vm);
    stack_push8(&vm->working, 0xA6);
    stack_push8(&vm->working, 0x42);
    run_single_opcode(vm, 0x11);
    assert(vm->ram[0x42] == 0xA6);
    assert(vm->working.pointer == 0);

    uxn_init(vm);
    stack_push16(&vm->working, 0xBEEF);
    stack_push8(&vm->working, 0xFF);
    run_single_opcode(vm, 0x31);
    assert(vm->ram[0x00FF] == 0xBE && vm->ram[0x0100] == 0xEF);
    assert(vm->working.pointer == 0);

    /* LDR / STR use signed offsets from the PC after the instruction. */
    uxn_init(vm);
    vm->ram[0x0103] = 0xD1;
    stack_push8(&vm->working, 0x02);
    run_single_opcode(vm, 0x12);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0xD1);

    uxn_init(vm);
    vm->ram[0x00FF] = 0x5C;
    stack_push8(&vm->working, 0xFF); /* -2 from PC 0x0101 */
    run_single_opcode(vm, 0x12);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x5C);

    uxn_init(vm);
    vm->ram[0x0103] = 0x12;
    vm->ram[0x0104] = 0x34;
    stack_push8(&vm->working, 0x02);
    run_single_opcode(vm, 0x32);
    assert(vm->working.pointer == 2);
    assert(vm->working.data[0] == 0x12 && vm->working.data[1] == 0x34);

    uxn_init(vm);
    stack_push8(&vm->working, 0xC7);
    stack_push8(&vm->working, 0x02);
    run_single_opcode(vm, 0x13);
    assert(vm->ram[0x0103] == 0xC7 && vm->working.pointer == 0);

    uxn_init(vm);
    stack_push16(&vm->working, 0xFACE);
    stack_push8(&vm->working, 0x02);
    run_single_opcode(vm, 0x33);
    assert(vm->ram[0x0103] == 0xFA && vm->ram[0x0104] == 0xCE);
    assert(vm->working.pointer == 0);

    /* LDA / STA consume 16-bit absolute addresses. */
    uxn_init(vm);
    vm->ram[0x4321] = 0x91;
    stack_push16(&vm->working, 0x4321);
    run_single_opcode(vm, 0x14);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x91);

    uxn_init(vm);
    vm->ram[0x4321] = 0xAB;
    vm->ram[0x4322] = 0xCD;
    stack_push16(&vm->working, 0x4321);
    run_single_opcode(vm, 0x34);
    assert(vm->working.pointer == 2);
    assert(vm->working.data[0] == 0xAB && vm->working.data[1] == 0xCD);

    uxn_init(vm);
    stack_push8(&vm->working, 0x7D);
    stack_push16(&vm->working, 0x4321);
    run_single_opcode(vm, 0x15);
    assert(vm->ram[0x4321] == 0x7D && vm->working.pointer == 0);

    uxn_init(vm);
    stack_push16(&vm->working, 0x2468);
    stack_push16(&vm->working, 0x4321);
    run_single_opcode(vm, 0x35);
    assert(vm->ram[0x4321] == 0x24 && vm->ram[0x4322] == 0x68);
    assert(vm->working.pointer == 0);

    /* DEI / DEO access device ports, including short adjacent-byte accesses. */
    uxn_init(vm);
    vm->dev[0x20] = 0xE1;
    stack_push8(&vm->working, 0x20);
    run_single_opcode(vm, 0x16);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0xE1);

    uxn_init(vm);
    vm->dev[0xFE] = 0x12;
    vm->dev[0xFF] = 0x34;
    stack_push8(&vm->working, 0xFE);
    run_single_opcode(vm, 0x36);
    assert(vm->working.pointer == 2);
    assert(vm->working.data[0] == 0x12 && vm->working.data[1] == 0x34);

    uxn_init(vm);
    stack_push8(&vm->working, 0xA9);
    stack_push8(&vm->working, 0x21);
    run_single_opcode(vm, 0x17);
    assert(vm->dev[0x21] == 0xA9 && vm->working.pointer == 0);

    uxn_init(vm);
    stack_push16(&vm->working, 0xCAFE);
    stack_push8(&vm->working, 0x30);
    run_single_opcode(vm, 0x37);
    assert(vm->dev[0x30] == 0xCA && vm->dev[0x31] == 0xFE);
    assert(vm->working.pointer == 0);

    uxn_destroy(vm);
    return 0;
}

int test_memory_opcode_modes(void) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    /* Return mode selects the return stack; keep mode preserves operands. */
    vm->ram[0x2345] = 0xA5;
    stack_push8(&vm->working, 0x77);
    stack_push16(&vm->ret, 0x2345);
    run_single_opcode(vm, 0xD4); /* LDAk,r */
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x77);
    assert(vm->ret.pointer == 3);
    assert(vm->ret.data[0] == 0x23 && vm->ret.data[1] == 0x45);
    assert(vm->ret.data[2] == 0xA5);

    uxn_init(vm);
    stack_push16(&vm->ret, 0xBEEF);
    stack_push16(&vm->ret, 0x2345);
    run_single_opcode(vm, 0xF5); /* STA2k,r */
    assert(vm->ram[0x2345] == 0xBE && vm->ram[0x2346] == 0xEF);
    assert(vm->ret.pointer == 4);
    assert(vm->ret.data[0] == 0xBE && vm->ret.data[1] == 0xEF);
    assert(vm->ret.data[2] == 0x23 && vm->ret.data[3] == 0x45);

    uxn_init(vm);
    vm->dev[0x40] = 0x12;
    vm->dev[0x41] = 0x34;
    stack_push8(&vm->ret, 0x40);
    run_single_opcode(vm, 0xF6); /* DEI2k,r */
    assert(vm->ret.pointer == 3);
    assert(vm->ret.data[0] == 0x40);
    assert(vm->ret.data[1] == 0x12 && vm->ret.data[2] == 0x34);

    uxn_init(vm);
    stack_push16(&vm->ret, 0xCAFE);
    stack_push8(&vm->ret, 0x50);
    run_single_opcode(vm, 0xD7); /* DEOk,r */
    assert(vm->dev[0x50] == 0xFE);
    assert(vm->ret.pointer == 3);
    assert(vm->ret.data[0] == 0xCA && vm->ret.data[1] == 0xFE);
    assert(vm->ret.data[2] == 0x50);

    uxn_destroy(vm);
    return 0;
}

int test_run_limits_and_wraparound(void) {
    Uxn *vm = uxn_create();
    assert(vm != NULL);

    vm->ram[0x0100] = 0x80;
    vm->ram[0x0101] = 0x42;
    vm->ram[0x0102] = 0x00;
    assert(uxn_run(vm, 0) == UXN_RUN_STEP_LIMIT);
    assert(uxn_get_pc(vm) == 0x0100 && vm->working.pointer == 0);
    assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);
    assert(uxn_get_pc(vm) == 0x0102 && vm->working.pointer == 1);
    assert(vm->working.data[0] == 0x42);
    assert(uxn_run(vm, 1) == UXN_RUN_BRK);
    assert(uxn_get_pc(vm) == 0x0103);

    uxn_init(vm);
    uxn_set_pc(vm, 0xFFFF);
    vm->ram[0xFFFF] = 0x80;
    vm->ram[0x0000] = 0x6D;
    vm->ram[0x0001] = 0x00;
    assert(uxn_run(vm, 1) == UXN_RUN_STEP_LIMIT);
    assert(uxn_get_pc(vm) == 0x0001);
    assert(vm->working.pointer == 1 && vm->working.data[0] == 0x6D);

    uxn_destroy(vm);
    return 0;
}
