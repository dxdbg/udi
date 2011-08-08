/*
 * Copyright (c) 2011, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Shared debugger and debuggee UDI implementation specific platforms

#define _GNU_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

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
        ssize_t num_read = read(fd, dest + total, length - total);
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
        ssize_t num_written = write(fd, src + total, length - total);
        if ( num_written < 0 ) {
            if ( errno == EINTR ) continue;
            return errno;
        }

        total += num_written;
    }

    return 0;
}
