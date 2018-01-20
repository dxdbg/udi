/*
 * Copyright (c) 2011-2018, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// UDI debuggee implementation common between all platforms

#ifndef _UDI_RT_X86_H
#define _UDI_RT_X86_H 1

#include <udis86.h>

#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

// disassembly interface //

/**
 * Gets the specified register from the context
 *
 * @param reg the register to retrieve
 * @param context the context
 *
 * @return the value from the register
 */
uint64_t get_register_ud_type(ud_type_t reg, const void *context);

/**
 * @param reg the register
 *
 * @return 0 if it isn't accessible; non-zero if it is
 */
int is_accessible_register(ud_type_t reg);

/**
 * Given the context, gets the flags register
 *
 * @param context the context containing the flags register
 *
 * @return the flags register in the context
 */
uint64_t get_flags(const void *context);

#ifdef __cplusplus
} // extern C
#endif

#endif
