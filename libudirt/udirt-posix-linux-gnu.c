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
// symbols defined to operate correct
extern void __nptl_create_event(void) __attribute__((weak));
extern void __nptl_death_event(void) __attribute__((weak));

// exported constants //

// library wrapping
void *UDI_RTLD_NEXT = RTLD_NEXT;

// register interface

#ifndef REG_EIP
#define REG_EIP -1
#endif
int EIP_OFFSET = REG_EIP;

#ifndef REG_ESP
#define REG_ESP -1
#endif
int ESP_OFFSET = REG_ESP;

#ifndef REG_RIP
#define REG_RIP -1
#endif
int RIP_OFFSET = REG_RIP;

#ifndef REG_RSP
#define REG_RSP -1
#endif
int RSP_OFFSET = REG_RSP;

#ifndef REG_RAX
#define REG_RAX -1
#endif
int RAX_OFFSET = REG_RAX;

// pthread events
void (*pthreads_create_event)(void) = __nptl_create_event;
void (*pthreads_death_event)(void) = __nptl_death_event;

/**
 * @return the kernel thread id for the currently executing thread
 */
unsigned long get_kernel_thread_id() {
    return (unsigned long)syscall(SYS_gettid);
}
