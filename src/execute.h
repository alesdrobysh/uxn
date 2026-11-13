#ifndef UXN_EXECUTE_H
#define UXN_EXECUTE_H

#include <stddef.h>
#include "uxn.h"
#include "opcode.h"
#include "memory.h"

typedef enum {
    UXN_RUN_BRK,
    UXN_RUN_UNSUPPORTED,
    UXN_RUN_STEP_LIMIT
} UxnRunResult;

UxnRunResult uxn_run(Uxn *vm, size_t max_steps);

#endif
