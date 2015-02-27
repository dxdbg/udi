/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI POSIX implementation (shared between debugger and debuggee library)

#ifndef _UDI_RT_POSIX_H_
#define _UDI_RT_POSIX_H_ 1

#include "udi.h"
#include "udi-common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Response request handling

int read_all(int fd, void *dest, size_t length);
int write_all(int fd, void *src, size_t length);

#ifdef __cplusplus
} // extern C
#endif

#endif
