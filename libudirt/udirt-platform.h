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

#ifndef _UDI_RT_PLATFORM_H
#define _UDI_RT_PLATFORM_H 1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LINUX)

#define _XOPEN_SOURCE 700

// Manually extracted from a system header, shouldn't change often
#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1L)
#endif

#ifndef REG_EIP
#define REG_EIP 14
#endif

#ifndef REG_ESP
#define REG_ESP 7
#endif

#ifndef REG_RIP
#define REG_RIP 16
#endif

#ifndef REG_RSP
#define REG_RSP 15
#endif

#ifndef REG_RAX
#define REG_RAX 13
#endif

#ifndef MAX_SIGNAL_NUM
#define MAX_SIGNAL_NUM 31
#endif

#else
#error Unknown platform
#endif

#ifdef __cplusplus
} // extern C
#endif

#endif
