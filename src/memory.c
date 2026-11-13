#include "memory.h"

uint8_t read8(const uint8_t *memory, uint16_t address) {
    return memory[address];
}

void write8(uint8_t *memory, uint16_t address, uint8_t value) {
    memory[address] = value;
}

uint16_t read16(const uint8_t *memory, uint16_t address) {
    uint8_t big = memory[address];

    uint16_t little_address = address + 1;
    uint8_t little = memory[little_address];

    uint16_t result = (uint16_t)((big << 8) + little);

    return result;
}

void write16(uint8_t *memory, uint16_t address, uint16_t value) {
    uint16_t little_address = address + 1;

    memory[address] = (uint8_t)(value >> 8);
    memory[little_address] = (uint8_t)value;
}

void fill_bytes(
    uint8_t *destination,
    size_t length,
    uint8_t value
) {
    for (size_t i = 0; i < length; i++) {
        *(destination + i) = value;
    }
}

void copy_bytes(
    uint8_t *destination,
    const uint8_t *source,
    size_t length
) {
    for (size_t i = 0; i < length; i++) {
        *(destination + i) = *(source + i);
    }
}

int equal_bytes(
    const uint8_t *a,
    const uint8_t *b,
    size_t length
) {
    for (size_t i = 0; i < length; i++) {
        if (*(a + i) != *(b + i)) {
            return 0;
        }
    }

    return 1;
}
