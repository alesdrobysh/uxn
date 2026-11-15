#include "execute.h"
#include "opcode.h"
#include "stack.h"

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
        } else if (op.kind == OP_STANDARD) {
            UxnStack * stack = op.return_mode ? &vm->ret : &vm->working;
            uint8_t saved_pointer = stack->pointer;

            if (op.base == 0x01) { // INC

                if (op.short_mode) {
                    uint16_t a = stack_pop16(stack);
                    
                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }
                    
                    uint16_t result = a + 1;
                    
                    stack_push16(stack, result);
                } else {
                    uint8_t a = stack_pop8(stack);
                    
                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }
                    
                    uint8_t result = a + 1;
                    
                    stack_push8(stack, result);
                }
            } else if (op.base == 0x18) { // ADD
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = a + b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = a + b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x19) { // SUB
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = a - b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = a - b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1A) { // MUL
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = (uint16_t)((uint32_t)a * b);

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = (uint8_t)((uint32_t)a * b);

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1B) { // DIV
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = b == 0 ? 0 : a / b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = b == 0 ? 0 : a / b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1C) { // AND
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = a & b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = a & b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1D) { // ORA
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = a | b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = a | b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1E) { // EOR
                if (op.short_mode) {
                    uint16_t b = stack_pop16(stack);
                    uint16_t a = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = a ^ b;

                    stack_push16(stack, result);
                } else {
                    uint8_t b = stack_pop8(stack);
                    uint8_t a = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = a ^ b;

                    stack_push8(stack, result);
                }
            } else if (op.base == 0x1F) { // SFT
                uint8_t control = stack_pop8(stack);
                uint8_t high_nibble = control >> 4;
                uint8_t low_nibble = control & 0x0F;

                if (op.short_mode) {
                    uint16_t value = stack_pop16(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint16_t result = (uint16_t)((value >> low_nibble) << high_nibble);
                    stack_push16(stack, result);
                } else {
                    uint8_t value = stack_pop8(stack);

                    if (op.keep_mode) {
                        stack->pointer = saved_pointer;
                    }

                    uint8_t result = (uint8_t)((value >> low_nibble) << high_nibble);
                    stack_push8(stack, result);
                }
            } else {
                vm->pc--;
                return UXN_RUN_UNSUPPORTED;
            }
        } else {
            vm->pc--;
            return UXN_RUN_UNSUPPORTED;
        }
    }   
    

    return UXN_RUN_STEP_LIMIT;
}
