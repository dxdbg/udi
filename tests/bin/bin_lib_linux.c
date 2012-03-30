#define _GNU_SOURCE 1

#include <unistd.h>
#include <sys/syscall.h>

#include "bin_lib.h"

long udi_get_thread_id(int pid) {
    return (long)syscall(SYS_gettid);
}
