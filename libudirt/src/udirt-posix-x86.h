/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _UDI_RT_POSIX_X86_H
#define _UDI_RT_POSIX_X86_H 1

#ifdef __cplusplus
extern "C" {
#endif

// register interface
extern int X86_GS_OFFSET;
extern int X86_FS_OFFSET;
extern int X86_ES_OFFSET;
extern int X86_DS_OFFSET;
extern int X86_EDI_OFFSET;
extern int X86_ESI_OFFSET;
extern int X86_EBP_OFFSET;
extern int X86_ESP_OFFSET;
extern int X86_EBX_OFFSET;
extern int X86_EDX_OFFSET;
extern int X86_ECX_OFFSET;
extern int X86_EAX_OFFSET;
extern int X86_CS_OFFSET;
extern int X86_SS_OFFSET;
extern int X86_EIP_OFFSET;
extern int X86_FLAGS_OFFSET;
extern int X86_64_R8_OFFSET;
extern int X86_64_R9_OFFSET;
extern int X86_64_R10_OFFSET;
extern int X86_64_R11_OFFSET;
extern int X86_64_R12_OFFSET;
extern int X86_64_R13_OFFSET;
extern int X86_64_R14_OFFSET;
extern int X86_64_R15_OFFSET;
extern int X86_64_RDI_OFFSET;
extern int X86_64_RSI_OFFSET;
extern int X86_64_RBP_OFFSET;
extern int X86_64_RBX_OFFSET;
extern int X86_64_RDX_OFFSET;
extern int X86_64_RAX_OFFSET;
extern int X86_64_RCX_OFFSET;
extern int X86_64_RSP_OFFSET;
extern int X86_64_RIP_OFFSET;
extern int X86_64_CSGSFS_OFFSET;
extern int X86_64_FLAGS_OFFSET;

#ifdef __cplusplus
} // extern C
#endif

#endif
