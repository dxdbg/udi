/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
void wait_for_single_step(udi_thread *thr);

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
