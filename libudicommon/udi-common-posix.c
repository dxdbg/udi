/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Shared debugger and debuggee UDI implementation specific platforms

#define _GNU_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "udi.h"
#include "udi-common.h"
#include "udi-common-posix.h"

// platform specific variables
const char *UDI_DS = "/";
const unsigned int DS_LEN = 1;
const char *DEFAULT_UDI_ROOT_DIR = "/tmp/udi";

int read_all(int fd, void *dest, size_t length) 
{
    size_t total = 0;
    while (total < length) {
        ssize_t num_read = read(fd, ((unsigned char*)dest) + total, 
                length - total);

        if ( num_read == 0 ) {
            // Treat end-of-file as a separate error
            return -1;
        }

        if (num_read < 0) {
            if (errno == EINTR) continue;
            return errno;
        }
        total += num_read;
    }

    return 0;
}

int write_all(int fd, void *src, size_t length) {
    size_t total = 0;
    while (total < length) {
        ssize_t num_written = write(fd, ((unsigned char *)src) + total, 
                length - total);
        if ( num_written < 0 ) {
            if ( errno == EINTR ) continue;
            return errno;
        }

        total += num_written;
    }

    return 0;
}
