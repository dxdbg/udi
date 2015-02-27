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

#if defined(LINUX)

#define _XOPEN_SOURCE 700

// pthreads support
#include <bits/pthreadtypes.h>

#else
#error Unknown platform
#endif

#ifdef __cplusplus
} // extern C
#endif

#endif
