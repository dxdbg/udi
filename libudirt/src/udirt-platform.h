/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _UDI_RT_PLATFORM_H
#define _UDI_RT_PLATFORM_H 1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(UNIX)

#define _XOPEN_SOURCE 700

typedef int udirt_fd;

#if defined(LINUX)

#define UDI_CONSTRUCTOR __attribute__((constructor))
#define UDI_WEAK __attribute__((weak))

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

// pthreads support
#include <pthread.h>
#include <bits/pthreadtypes.h>

#endif /* LINUX */

#if defined(DARWIN)

#define UDI_CONSTRUCTOR __attribute__((constructor))
#define UDI_WEAK __attribute__((weak))
#define _DARWIN_C_SOURCE 1

#include <pthread.h>

#endif /* DARWIN */

#else /* UNIX / WINDOWS */

typedef void * udirt_fd;

#endif /* WINDOWS */

#ifdef __cplusplus
} // extern C
#endif

#endif
