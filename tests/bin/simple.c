#include <stdio.h>
#include <stdlib.h>

#include "bin_lib.h"

int function1() {
    bin_printf("In function1\n");
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    init_bin();

    bin_printf("Before function1\n");
    function1();
    bin_printf("After function1\n");

    return EXIT_FAILURE;
}
