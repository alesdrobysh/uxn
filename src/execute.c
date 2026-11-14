#include "execute.h"
#include "opcode.h"
#include "stack.h"
#include <stdio.h>

UxnRunResult uxn_run(Uxn *vm, size_t max_steps) {
    for  (size_t i = 0; i < max_steps; i++)
    {
        uint8_t next = vm->ram[vm->pc];
        vm->pc++;
        DecodedOpcode op = decode_opcode(next);

        if (op.kind == OP_LIT) {
            uint8_t value = read8(vm->ram, vm->pc);
            vm->pc++;
            stack_push8(&vm->working, value);
        } else if (op.kind == OP_LIT2) {
            uint16_t value = read16(vm->ram, vm->pc);
            vm->pc += 2;
            stack_push16(&vm->working, value);
        } else if (op.kind == OP_LITR) {
            uint8_t value = read8(vm->ram, vm->pc);
            vm->pc++;
            stack_push8(&vm->ret, value);
        } else if (op.kind == OP_LIT2R) {
            uint16_t value = read16(vm->ram, vm->pc);
            vm->pc += 2;
            stack_push16(&vm->ret, value);
        } else if (op.kind == OP_JCI) {
            uint8_t stack_value = stack_pop8(&vm->working);
            uint16_t shift = read16(vm->ram, vm->pc);
            vm->pc += 2;

            if (stack_value != 0) {
                vm->pc += shift;
            }
        } else if (op.kind == OP_JSI) {
            uint16_t shift = read16(vm->ram, vm->pc);
            vm->pc += 2;
            stack_push16(&vm->ret, vm->pc);
            vm->pc += shift;
        } else if (op.kind == OP_JMI) {
            uint16_t shift = read16(vm->ram, vm->pc);
            vm->pc += 2;
            vm->pc += shift;
        } else if (op.kind == OP_BRK) {
            return UXN_RUN_BRK;
        } else {
            vm->pc--;
            return UXN_RUN_UNSUPPORTED;
        }
    }   

    return UXN_RUN_STEP_LIMIT;
}
