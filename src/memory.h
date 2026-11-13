#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdlib.h>

uint8_t read8(const uint8_t *memory, uint16_t address);
void write8(uint8_t *memory, uint16_t address, uint8_t value);
uint16_t read16(const uint8_t *memory, uint16_t address);
void write16(uint8_t *memory, uint16_t address, uint16_t value);
void fill_bytes(
    uint8_t *destination,
    size_t length,
    uint8_t value
);
void copy_bytes(
    uint8_t *destination,
    const uint8_t *source,
    size_t length
);
int equal_bytes(
    const uint8_t *a,
    const uint8_t *b,
    size_t length
);

#endif
