#include <assert.h>
#include "execute.h"

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
    assert(vm->ret.pointer == 255);

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}
