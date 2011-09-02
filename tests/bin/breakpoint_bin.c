#include <stdio.h>
#include <stdlib.h>

int function1() {
    printf("In function1\n");
    return EXIT_SUCCESS;
}

extern void init_udi_rt();

int main(int argc, char *argv[]) {
    // TODO remove this
    init_udi_rt();

    printf("Before function1\n");
    function1();
    printf("After function1\n");
    
    return EXIT_SUCCESS;
}
