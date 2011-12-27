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
    
    return EXIT_SUCCESS;
}
