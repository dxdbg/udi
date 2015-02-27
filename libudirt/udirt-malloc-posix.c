/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "udirt.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

/**
 * Gets mapped memory from the OS
 *
 * @param length the size of memory area to map (in bytes)
 *
 * @return the mapped memory
 */
unsigned char *map_mem(size_t length) {
    void * ret = mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS 
            | MAP_PRIVATE, -1, 0);

    if ( ret == MAP_FAILED ) {
        udi_printf("mmap failed: %s\n", strerror(errno));
        return NULL;
    }

    return ret;
}

/**
 * Returns a mapped memory region to the OS
 *
 * @param addr the address of the mapped memory region
 * @param length the size of the memory area to map (in bytes)
 *
 * @return non-zero on failure
 */
int unmap_mem(void *addr, size_t length) {
    return munmap(addr, length);
}
