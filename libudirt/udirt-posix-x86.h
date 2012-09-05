/*
 * Copyright (c) 2011-2012, Dan McNulty
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the UDI project nor the
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

#ifdef __cplusplus
} // extern C
#endif

#endif
