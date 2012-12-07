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

#define _GNU_SOURCE

#include <unistd.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <ucontext.h>

// These symbols are not exported through the dynamic symbol table
// but it turns out that GDB relies on libpthread having these
// symbols defined to operate correctly
extern void __nptl_create_event(void) __attribute__((weak));
extern void __nptl_death_event(void) __attribute__((weak));

// exported constants //

// library wrapping
void *UDI_RTLD_NEXT = RTLD_NEXT;

// register interface

// x86

#ifndef REG_GS
#define REG_GS -1
#endif
int X86_GS_OFFSET = REG_GS;

#ifndef REG_FS
#define REG_FS -1
#endif
int X86_FS_OFFSET = REG_FS;

#ifndef REG_ES
#define REG_ES -1
#endif
int X86_ES_OFFSET = REG_ES;

#ifndef REG_DS
#define REG_DS -1
#endif
int X86_DS_OFFSET = REG_DS;

#ifndef REG_EDI
#define REG_EDI -1
#endif
int X86_EDI_OFFSET = REG_EDI;

#ifndef REG_ESI
#define REG_ESI -1
#endif
int X86_ESI_OFFSET = REG_ESI;

#ifndef REG_EBP
#define REG_EBP -1
#endif
int X86_EBP_OFFSET = REG_EBP;

#ifndef REG_ESP
#define REG_ESP -1
#endif
int X86_ESP_OFFSET = REG_ESP;

#ifndef REG_EBX
#define REG_EBX -1
#endif
int X86_EBX_OFFSET = REG_EBX;


#ifndef REG_EDX
#define REG_EDX -1
#endif
int X86_EDX_OFFSET = REG_EDX;

#ifndef REG_ECX
#define REG_ECX -1
#endif
int X86_ECX_OFFSET = REG_ECX;

#ifndef REG_EAX
#define REG_EAX -1
#endif
int X86_EAX_OFFSET = REG_EAX;

#ifndef REG_CS
#define REG_CS -1
#endif
int X86_CS_OFFSET = REG_CS;

#ifndef REG_SS
#define REG_SS -1
#endif
int X86_SS_OFFSET = REG_SS;

#ifndef REG_EIP
#define REG_EIP -1
#endif
int X86_EIP_OFFSET = REG_EIP;

#ifndef REG_EFL
#define REG_EFL -1
#endif
int X86_FLAGS_OFFSET = REG_EFL;

// x86_64

#ifndef REG_R8
#define REG_R8 -1
#endif
int X86_64_R8_OFFSET = REG_R8;

#ifndef REG_R9
#define REG_R9 -1
#endif
int X86_64_R9_OFFSET = REG_R9;

#ifndef REG_R10
#define REG_R10 -1
#endif
int X86_64_R10_OFFSET = REG_R10;

#ifndef REG_R11
#define REG_R11 -1
#endif
int X86_64_R11_OFFSET = REG_R11;

#ifndef REG_R12
#define REG_R12 -1
#endif
int X86_64_R12_OFFSET = REG_R12;

#ifndef REG_R13
#define REG_R13 -1
#endif
int X86_64_R13_OFFSET = REG_R13;

#ifndef REG_R14
#define REG_R14 -1
#endif
int X86_64_R14_OFFSET = REG_R14;

#ifndef REG_R15
#define REG_R15 -1
#endif
int X86_64_R15_OFFSET = REG_R15;

#ifndef REG_RDI
#define REG_RDI -1
#endif
int X86_64_RDI_OFFSET = REG_RDI;

#ifndef REG_RSI
#define REG_RSI -1
#endif
int X86_64_RSI_OFFSET = REG_RSI;

#ifndef REG_RBP
#define REG_RBP -1
#endif
int X86_64_RBP_OFFSET = REG_RBP;

#ifndef REG_RBX
#define REG_RBX -1
#endif
int X86_64_RBX_OFFSET = REG_RBX;

#ifndef REG_RDX
#define REG_RDX -1
#endif
int X86_64_RDX_OFFSET = REG_RDX;

#ifndef REG_RAX
#define REG_RAX -1
#endif
int X86_64_RAX_OFFSET = REG_RAX;

#ifndef REG_RCX
#define REG_RCX -1
#endif
int X86_64_RCX_OFFSET = REG_RCX;

#ifndef REG_RSP
#define REG_RSP -1
#endif
int X86_64_RSP_OFFSET = REG_RSP;

#ifndef REG_RIP
#define REG_RIP -1
#endif
int X86_64_RIP_OFFSET = REG_RIP;

#ifndef REG_CSGSFS
#define REG_CSGSFS -1
#endif
int X86_64_CSGSFS_OFFSET = REG_CSGSFS;

int X86_64_FLAGS_OFFSET = REG_EFL;

// pthread events
void (*pthreads_create_event)(void) = __nptl_create_event;
void (*pthreads_death_event)(void) = __nptl_death_event;

/**
 * @return the kernel thread id for the currently executing thread
 */
unsigned long get_kernel_thread_id() {
    return (unsigned long)syscall(SYS_gettid);
}
