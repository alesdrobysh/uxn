#include "execute.h"

int main(void) {
    Uxn *vm = uxn_create();

    if (vm == NULL) {
        return 1;
    }

    uxn_destroy(vm);
    vm = NULL;

    return 0;
}
