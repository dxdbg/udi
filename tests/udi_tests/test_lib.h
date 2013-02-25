/*
 * Copyright (c) 2011-2013, Dan McNulty
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

#ifndef _TEST_LIB_H_
#define _TEST_LIB_H_ 1

#include <iostream>
#include <set>
#include <map>
#include <vector>

#include "libudi.h"
#include "libuditest.h"

/**
 * Mix-in for implementing an event callback
 */
class EventCallback {
    public:
        virtual void operator()(udi_event *event) = 0;
};

void wait_for_events(udi_process *proc, EventCallback &callback);
void wait_for_events(const std::set<udi_process *> &procs, EventCallback &callback);

void wait_for_exit(udi_thread *thr, int expected_status);
void wait_for_breakpoint(udi_thread *thr, udi_address breakpoint);

udi_thread *wait_for_thread_create(udi_process *proc);
void wait_for_thread_death(udi_thread *thr);

void validate_thread_state(udi_process *proc, udi_thread_state_e state);
void validate_thread_state(const std::map<udi_thread *, udi_thread_state_e> &states);

void release_debuggee_threads(udi_process *proc);
void wait_for_debuggee_pipe(udi_process *proc);
void cleanup_debuggee_pipe(udi_process *proc);

std::ostream& operator<<(std::ostream &os, udi_process *proc);

#define assert_no_error(proc, e) test_assert_msg(get_last_error_message(proc), e == UDI_ERROR_NONE)

#endif
