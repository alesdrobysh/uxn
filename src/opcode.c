#include "opcode.h"
#include <stdlib.h>

const char *opcode_name(uint8_t base) {
    if (base < 0x01 || base > 0x1F) return NULL;

    char* names[] = {
        "",
        /* 01 */ "INC",   /* 02 */ "POP",   /* 03 */ "NIP",   /* 04 */ "SWP",
        /* 05 */ "ROT",   /* 06 */ "DUP",   /* 07 */ "OVR",   /* 08 */ "EQU",
        /* 09 */ "NEQ",   /* 0A */ "GTH",   /* 0B */ "LTH",   /* 0C */ "JMP",
        /* 0D */ "JCN",   /* 0E */ "JSR",   /* 0F */ "STH",   /* 10 */ "LDZ",
        /* 11 */ "STZ",   /* 12 */ "LDR",   /* 13 */ "STR",   /* 14 */ "LDA",
        /* 15 */ "STA",   /* 16 */ "DEI",   /* 17 */ "DEO",   /* 18 */ "ADD",
        /* 19 */ "SUB",   /* 1A */ "MUL",   /* 1B */ "DIV",   /* 1C */ "AND",
        /* 1D */ "ORA",   /* 1E */ "EOR",   /* 1F */ "SFT"
    };

    return names[base];

}

DecodedOpcode decode_opcode(uint8_t raw) {
    DecodedOpcode opcode = {0};

    if (raw == 0x00) {
        opcode.kind = OP_BRK;
    } else if (raw == 0x20) {
        opcode.kind = OP_JCI;
    } else if (raw == 0x40) {
        opcode.kind = OP_JMI;
    } else if (raw == 0x60) {
        opcode.kind = OP_JSI;
    } else if (raw == 0x80) {
        opcode.kind = OP_LIT;
    } else if (raw == 0xA0) {
        opcode.kind = OP_LIT2;
    } else if (raw == 0xC0) {
        opcode.kind = OP_LITR;
    } else if (raw == 0xE0) {
        opcode.kind = OP_LIT2R;
    } else {
        opcode.kind = OP_STANDARD;
        opcode.base = raw & 0x1F;
        opcode.keep_mode = (raw & 0x80) > 0;
        opcode.return_mode = (raw & 0x40) > 0;
        opcode.short_mode = (raw & 0x20) > 0;
    }

    return opcode;
}
