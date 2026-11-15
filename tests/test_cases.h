#ifndef TEST_CASES_H
#define TEST_CASES_H

int test_vm_structure(void);
int test_run_lit_lit2(void);
int test_run_jmi(void);
int test_run_jci(void);
int test_run_jsi(void);
int test_arithmetic(void);
int test_decode_opcode(void);
int test_memory_helpers(void);
int test_vm_initialization_and_reset(void);
int test_memory_device_opcodes(void);
int test_memory_opcode_modes(void);
int test_run_limits_and_wraparound(void);
int test_stack_opcodes(void);
int test_comparison_opcodes(void);
int test_control_flow_opcodes(void);
int test_control_opcode_modes(void);
int test_immediate_control_and_literal_opcodes(void);
int test_decode_opcode_exhaustive(void);

#endif
