#include <stdio.h>
#include <stdlib.h>

int function1() {
    printf("In function1\n");
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    printf("Before function1\n");
    function1();
    printf("After function1\n");

    // This is a temporary workaround until
    // can detect implicit exits via libc
    exit(EXIT_SUCCESS);
}
