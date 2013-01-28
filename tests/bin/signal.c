#include <stdio.h>
#include <stdlib.h>

#include "bin_lib.h"

#ifndef UNIX

int main(int argc, char *argv[]) {
    init_bin();

    return EXIT_SUCCESS;
}

#else

#include <signal.h>
#include <unistd.h>

volatile int wait_flag = 0;

void signal_handler(int signal) {
    bin_printf("Received signal %d\n", signal);
    sleep(5);
    bin_printf("Done sleeping\n");
    kill(getpid(), SIGUSR1);
}

int main(int argc, char *argv[]) {
    init_bin();

    signal(SIGUSR1, signal_handler);

    while (!wait_flag) {
        pause();
    }

    return EXIT_SUCCESS;
}

#endif
