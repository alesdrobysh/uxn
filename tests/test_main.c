#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "execute.h"

int main(void) {
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

    vm = uxn_create();

    vm->ram[0x0100] = 0x80;
    vm->ram[0x0101] = 0x2A;
    vm->ram[0x0102] = 0xA0;
    vm->ram[0x0103] = 0x12;
    vm->ram[0x0104] = 0x34;
    vm->ram[0x0105] = 0x00;
    
    uxn_run(vm, 5);

    assert(vm->working.data[0] == 0x2A);
    assert(vm->working.data[1] == 0x12);
    assert(vm->working.data[2] == 0x34);
    assert(vm->working.pointer == 3);

    uxn_destroy(vm);

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

    puts("All tests passed!");

    return 0;
}
