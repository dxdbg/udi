/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// This needs to be included first to set feature macros
#include "udirt-platform.h"

#include <sys/syscall.h>
#include <unistd.h>

#include "udirt-posix.h"

uint64_t get_kernel_thread_id() {
    return (uint64_t)syscall(SYS_thread_selfid);
}
