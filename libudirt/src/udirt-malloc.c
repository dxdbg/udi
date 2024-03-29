/*
 * Copyright (c) 2011-2023, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Configure dlmalloc
#define USE_LOCKS 0
#define MMAP_PROT            (PROT_READ|PROT_WRITE|PROT_EXEC)
#define HAVE_MORECORE 0
#define DLMALLOC_EXPORT
#define USE_DL_PREFIX 1

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"

#include "dlmalloc.c"

#pragma clang diagnostic pop

#include "udirt.h"

/**
 * Internal UDI RT heap free
 */
void udi_free(void *in_ptr) {

    dlfree(in_ptr);
}

void *udi_malloc(size_t length) {

    return dlmalloc(length);
}

void *udi_calloc(size_t count, size_t size) {

    return dlcalloc(count, size);
}

void *udi_realloc(void *ptr, size_t length) {

    return dlrealloc(ptr, length);
}
