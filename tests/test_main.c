#include <stdio.h>
#include "test_cases.h"

int main(void) {
    int result =
        test_vm_structure() ||
        test_run_lit_lit2() ||
        test_run_jmi() ||
        test_run_jci() ||
        test_run_jsi() ||
        test_arithmetic() ||
        test_stack_opcodes() ||
        test_decode_opcode();

    puts("All tests passed!");

    return result;
}
