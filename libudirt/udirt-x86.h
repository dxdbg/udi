/*
 * Copyright (c) 2011-2015, UDI Contributors
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

#ifdef __cplusplus
extern "C" {
#endif

// disassembly interface
unsigned long get_register_ud_type(ud_type_t reg, const void *context);
int is_accessible_register(ud_type_t reg);
unsigned long get_flags(const void *context);

#ifdef __cplusplus
} // extern C
#endif

#endif
