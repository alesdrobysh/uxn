#include <assert.h>
#include "execute.h"
#include "opcode.h"
#include "stack.h"

static inline void restore_if_keep(UxnStack *stack, uint8_t saved, bool keep) {
    if (keep) stack->pointer = saved;
}

static void stack_push(UxnStack *stack, uint16_t value, bool short_mode) {
    if (short_mode) {
        stack_push16(stack, value);
    } else {
        stack_push8(stack, (uint8_t)value);
    }
}

static uint16_t stack_pop(UxnStack *stack, bool short_mode) {
    if (short_mode) {
        return stack_pop16(stack);
    } else {
        return stack_pop8(stack);
    }
}

static uint8_t fetch8(Uxn *vm) {
    uint8_t value = read8(vm->ram, vm->pc);
    vm->pc++;
    return value;
}

static uint16_t fetch16(Uxn *vm) {
    uint16_t value = read16(vm->ram, vm->pc);
    vm->pc += 2;
    return value;
}

static uint16_t alu_op(uint8_t op_base, uint16_t a, uint16_t b) {
    switch (op_base)
    {
        case 0x08: return a == b;
        case 0x09: return a != b;
        case 0x0A: return a > b;
        case 0x0B: return a < b;
        case 0x18: return a + b;
        case 0x19: return a - b;
        case 0x1A: return (uint16_t)((uint32_t)a * b);
        case 0x1B: return b == 0 ? 0 : a / b;
        case 0x1C: return a & b;
        case 0x1D: return a | b;
        case 0x1E: return a ^ b;
        
        default: assert(0); return 0;
    }
}

static void execute_alu_op(UxnStack * stack, DecodedOpcode op) {
    uint8_t saved_pointer = stack->pointer;

    uint16_t b = stack_pop(stack, op.short_mode);
    uint16_t a = stack_pop(stack, op.short_mode);

    restore_if_keep(stack, saved_pointer, op.keep_mode);
    
    uint16_t result = alu_op(op.base, a, b);

    stack_push(stack, result, op.short_mode);
}

static void execute_stack_op(UxnStack *stack, DecodedOpcode op) {
    uint8_t saved_pointer = stack->pointer;

    switch (op.base)
    {
        case 0x02: { // POP
            stack_pop(stack, op.short_mode);
            restore_if_keep(stack, saved_pointer, op.keep_mode);
            break;
        } case 0x03: { // NIP
            uint16_t value_to_keep = stack_pop(stack, op.short_mode);
            stack_pop(stack, op.short_mode);

            restore_if_keep(stack, saved_pointer, op.keep_mode);
            
            stack_push(stack, value_to_keep, op.short_mode);
            break;
        } case 0x04: { // SWP
            uint16_t b = stack_pop(stack, op.short_mode);
            uint16_t a = stack_pop(stack, op.short_mode);

            restore_if_keep(stack, saved_pointer, op.keep_mode);
        
            stack_push(stack, b, op.short_mode);
            stack_push(stack, a, op.short_mode);
            break;
        } case 0x05: { // ROT
            uint16_t c = stack_pop(stack, op.short_mode);
            uint16_t b = stack_pop(stack, op.short_mode);
            uint16_t a = stack_pop(stack, op.short_mode);

            restore_if_keep(stack, saved_pointer, op.keep_mode);
        
            stack_push(stack, b, op.short_mode);
            stack_push(stack, c, op.short_mode);
            stack_push(stack, a, op.short_mode);
            break;
        } case 0x06: { // DUP
            uint16_t a = stack_pop(stack, op.short_mode);

            restore_if_keep(stack, saved_pointer, op.keep_mode);
        
            stack_push(stack, a, op.short_mode);
            stack_push(stack, a, op.short_mode);
            break;
        } case 0x07: { // OVR
            uint16_t b = stack_pop(stack, op.short_mode);
            uint16_t a = stack_pop(stack, op.short_mode);

            restore_if_keep(stack, saved_pointer, op.keep_mode);
        
            stack_push(stack, a, op.short_mode);
            stack_push(stack, b, op.short_mode);
            stack_push(stack, a, op.short_mode);
            break;
        }
        
        default: assert(0);
    }
}

UxnRunResult uxn_run(Uxn *vm, size_t max_steps) {
    for  (size_t i = 0; i < max_steps; i++)
    {
        uint8_t next = fetch8(vm);
        DecodedOpcode op = decode_opcode(next);

        if (op.kind == OP_LIT) {
            uint8_t value = fetch8(vm);
            stack_push8(&vm->working, value);
        } else if (op.kind == OP_LIT2) {
            uint16_t value = fetch16(vm);
            stack_push16(&vm->working, value);
        } else if (op.kind == OP_LITR) {
            uint8_t value = fetch8(vm);
            stack_push8(&vm->ret, value);
        } else if (op.kind == OP_LIT2R) {
            uint16_t value = fetch16(vm);
            stack_push16(&vm->ret, value);
        } else if (op.kind == OP_JCI) {
            uint8_t stack_value = stack_pop8(&vm->working);
            uint16_t shift = fetch16(vm);

            if (stack_value != 0) {
                vm->pc += shift;
            }
        } else if (op.kind == OP_JSI) {
            uint16_t shift = fetch16(vm);
            stack_push16(&vm->ret, vm->pc);
            vm->pc += shift;
        } else if (op.kind == OP_JMI) {
            uint16_t shift = fetch16(vm);
            vm->pc += shift;
        } else if (op.kind == OP_BRK) {
            return UXN_RUN_BRK;
        } else if (op.kind == OP_STANDARD) {
            UxnStack * stack = op.return_mode ? &vm->ret : &vm->working;
            uint8_t saved_pointer = stack->pointer;

            if (op.base == 0x01) { // INC
                uint16_t a = stack_pop(stack, op.short_mode);

                restore_if_keep(stack, saved_pointer, op.keep_mode);

                uint16_t result = a + 1;

                stack_push(stack, result, op.short_mode);
            } else if (op.base >= 0x02 && op.base <= 0x07) {
                execute_stack_op(stack, op);
            } else if (op.base >= 0x08 && op.base <= 0x0B) {
                execute_alu_op(stack, op);
            } else if (op.base >= 0x18 && op.base <= 0x1E) {
                execute_alu_op(stack, op);
            } else if (op.base == 0x1F) { // SFT
                uint8_t control = stack_pop8(stack);
                uint8_t high_nibble = control >> 4;
                uint8_t low_nibble = control & 0x0F;

                uint16_t value = stack_pop(stack, op.short_mode);

                restore_if_keep(stack, saved_pointer, op.keep_mode);

                uint16_t result = (uint16_t)((value >> low_nibble) << high_nibble);

                stack_push(stack, result, op.short_mode);
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
