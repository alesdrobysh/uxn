#include "uxn.h"
#include "memory.h"
#include <stdlib.h>

void uxn_init(Uxn *vm) {
    fill_bytes(vm->ram, UXN_RAM_SIZE, 0);
    fill_bytes(vm->dev, UXN_DEV_SIZE, 0);
    fill_bytes(vm->working.data, UXN_STACK_SIZE, 0);
    fill_bytes(vm->ret.data, UXN_STACK_SIZE, 0);
    vm->working.pointer = 0;
    vm->ret.pointer = 0;
    vm->pc = 0x0100;
}

void uxn_set_pc(Uxn *vm, uint16_t address) {
    vm->pc = address;
}

uint16_t uxn_get_pc(const Uxn *vm) {
    return vm->pc;
}

Uxn *uxn_create(void) {
    Uxn *vm = malloc(sizeof *vm);

    if (vm == NULL) {
        return NULL;
    }

    uxn_init(vm);
    
    return vm;
}

void uxn_destroy(Uxn *vm) {
    free(vm);
}
