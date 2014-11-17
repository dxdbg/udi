#include <stdio.h>
#include <stdlib.h>

#include "bin_lib.h"

int limit = 100;
int j = 0;

static
void function2() {
    for (int i = 0; i < limit; ++i) {
        j++;
    }
}

int function1() {
    bin_printf("In function1\n");
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    init_bin();

    bin_printf("Before function1\n");
    function1();
    bin_printf("After function1\n");

    bin_printf("Before function2\n");
    function2();
    bin_printf("After function2\n");

    return EXIT_FAILURE;
}
