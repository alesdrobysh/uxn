#ifndef UXN_H
#define UXN_H

#include <stdint.h>
#include "stack.h"

#define UXN_RAM_SIZE 65536
#define UXN_DEV_SIZE 256

typedef struct {
    uint8_t ram[UXN_RAM_SIZE];
    uint16_t pc;
    UxnStack working;
    UxnStack ret;
    uint8_t dev[UXN_DEV_SIZE];
} Uxn;

Uxn *uxn_create(void);
void uxn_destroy(Uxn *vm);
void uxn_init(Uxn *vm);

void uxn_set_pc(Uxn *vm, uint16_t address);
uint16_t uxn_get_pc(const Uxn *vm);

#endif
