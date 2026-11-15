#include <assert.h>
#include <string.h>
#include "execute.h"

int test_decode_opcode(void) {
    DecodedOpcode op;

    op = decode_opcode(0x01);
    assert(op.kind == OP_STANDARD);
    assert(op.base == 0x01);
    assert(!op.short_mode);
    assert(!op.return_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x21);
    assert(op.base == 0x01);
    assert(op.short_mode);
    assert(!op.return_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x41);
    assert(op.return_mode);
    assert(!op.short_mode);
    assert(!op.keep_mode);

    op = decode_opcode(0x81);
    assert(op.keep_mode);
    assert(!op.short_mode);
    assert(!op.return_mode);

    op = decode_opcode(0xF8);
    assert(op.kind == OP_STANDARD);
    assert(op.base == 0x18);
    assert(op.short_mode);
    assert(op.return_mode);
    assert(op.keep_mode);

    assert(decode_opcode(0x00).kind == OP_BRK);
    assert(decode_opcode(0x20).kind == OP_JCI);
    assert(decode_opcode(0x40).kind == OP_JMI);
    assert(decode_opcode(0x60).kind == OP_JSI);
    assert(decode_opcode(0x80).kind == OP_LIT);
    assert(decode_opcode(0xA0).kind == OP_LIT2);
    assert(decode_opcode(0xC0).kind == OP_LITR);
    assert(decode_opcode(0xE0).kind == OP_LIT2R);

    assert(strcmp(opcode_name(0x01), "INC") == 0);
    assert(strcmp(opcode_name(0x18), "ADD") == 0);
    assert(strcmp(opcode_name(0x1F), "SFT") == 0);

    assert(opcode_name(0x00) == NULL);
    assert(opcode_name(0x20) == NULL);

    return 0;
}
