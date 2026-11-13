#ifndef UXN_STACK_H
#define UXN_STACK_H

#include <stdint.h>

#define UXN_STACK_SIZE 256

typedef struct {
    uint8_t data[UXN_STACK_SIZE];
    uint8_t pointer;
} UxnStack;

void stack_push8(UxnStack *stack, uint8_t value);
uint8_t stack_pop8(UxnStack *stack);

void stack_push16(UxnStack *stack, uint16_t value);
uint16_t stack_pop16(UxnStack *stack);

#endif
