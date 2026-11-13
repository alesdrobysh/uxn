#include "stack.h"

void stack_push8(UxnStack *stack, uint8_t value) {
    stack->data[stack->pointer] = value;
    uint8_t new_pointer = stack->pointer + 1;
    stack->pointer = new_pointer;
}

uint8_t stack_pop8(UxnStack *stack) {
    stack->pointer = stack->pointer - 1;
    uint8_t return_data = stack->data[stack->pointer];
    return return_data;
}

void stack_push16(UxnStack *stack, uint16_t value) {
    stack_push8(stack, value >> 8);
    stack_push8(stack, (uint8_t)value);
}

uint16_t stack_pop16(UxnStack *stack) {
    uint8_t low = stack_pop8(stack);
    uint8_t high = stack_pop8(stack);

    return (uint16_t)((high << 8) + low);
}
