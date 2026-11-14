#ifndef OPCODE_H
#define OPCODE_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    OP_STANDARD,
    OP_BRK,
    OP_JCI,
    OP_JMI,
    OP_JSI,
    OP_LIT,
    OP_LIT2,
    OP_LITR,
    OP_LIT2R
} OpcodeKind;

typedef struct {
    OpcodeKind kind;
    uint8_t base;
    bool short_mode;
    bool return_mode;
    bool keep_mode;
} DecodedOpcode;

const char *opcode_name(uint8_t base);
const char *format_opcode_kind(OpcodeKind kind);
DecodedOpcode decode_opcode(uint8_t raw);
void debug_opcode(DecodedOpcode op);

#endif
