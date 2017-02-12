/*
 * Copyright (c) 2011-2015, UDI Contributors
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

// pthreads support
#include <bits/pthreadtypes.h>

#endif /* LINUX */

#else /* UNIX */

#error Unsupported platform

#endif /* UNIX */

#ifdef __cplusplus
} // extern C
#endif

#endif
