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


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <map>
#include <typeinfo>

#include "libuditest.h"
#include "libudi.h"
#include "test_lib.h"

using std::cout;
using std::endl;
using std::stringstream;
using std::ostream;
using std::map;
using std::pair;
using std::make_pair;

ostream& operator<<(ostream &os, udi_process *proc) {
    os << "process[" << get_proc_pid(proc) << "]";

    return os;
}

/**
 * Waits for a set of events to happen in the specified collection of threads, ordered by their
 * processes
 *
 * @param events the specification for expected events
 * 
 * @return the events that occurred, ordered by their threads
 */ 
map<udi_thread *, udi_event *> wait_for_events(
        const map<udi_process *, map<udi_thread *, udi_event_type> > &events) {

    map<udi_thread *, udi_event *> results;
    for(map<udi_process *, map<udi_thread *, udi_event_type> >::const_iterator i = events.begin();
            i != events.end();
            ++i)
    {
        udi_process *proc = i->first;
        map<udi_thread *, udi_event_type> thr_events = i->second;

        while (!thr_events.empty()) {
            udi_event *event = wait_for_events(&proc, 1);
            test_assert(event != NULL);

            map<udi_thread *, udi_event_type>::iterator iter = thr_events.find(event->thr);
            test_assert(iter != thr_events.end());
            test_assert(iter->second == event->event_type);

            results.insert(make_pair(event->thr, event));
            thr_events.erase(iter);

            udi_error_e result = continue_process(proc);
            assert_no_error(result);
        }
    }

    return results;
}

/**
 * Waits for a thread to be created in the specified process
 *
 * @param proc the process to wait on
 *
 * @return the thread that was created
 */
udi_thread *wait_for_thread_create(udi_process *proc) {
    udi_event *event = wait_for_events(&proc, 1);

    test_assert(event != NULL);
    test_assert(event->proc == proc);
    test_assert(event->event_type == UDI_EVENT_THREAD_CREATE);
    test_assert(event->next_event == NULL);

    udi_thread *thr = event->thr;
    free_event_list(event);

    return thr;
}

/**
 * Waits for the death of the specified thread
 *
 * @param thr the thread
 */
void wait_for_thread_death(udi_thread *thr) {
    udi_process *proc = get_process(thr);
    udi_event *event = wait_for_events(&proc, 1);

    test_assert(event->proc == proc);
    test_assert(event->thr == thr);
    test_assert(event->event_type == UDI_EVENT_THREAD_DEATH);

    free_event_list(event);
}

/**
 * Waits for the specified process to hit the expected breakpoint
 *
 * @param thr the thread
 */
void wait_for_breakpoint(udi_thread *thr, udi_address breakpoint) {

    udi_process *proc = get_process(thr);

    udi_event *event = wait_for_events(&proc, 1);

    test_assert(event->proc == proc);
    test_assert(event->thr == thr);
    test_assert(event->event_type == UDI_EVENT_BREAKPOINT);

    udi_event_breakpoint *proc_breakpoint = (udi_event_breakpoint *)event->event_data;
    test_assert(proc_breakpoint->breakpoint_addr == breakpoint);
    test_assert(event->next_event == NULL);

    free_event_list(event);
}

/**
 * Waits for the specified process to exit
 *
 * @param thr the thread that will trigger the exit
 * @param expected_status the expected status for the process exit
 */
void wait_for_exit(udi_thread *thr, int expected_status) {

    udi_process *proc = get_process(thr);

    udi_event *event = wait_for_events(&proc, 1);
    test_assert(event->proc == proc);
    test_assert(event->thr == thr);
    test_assert(event->event_type == UDI_EVENT_PROCESS_EXIT);

    udi_event_process_exit *proc_exit = (udi_event_process_exit *)event->event_data;
    test_assert(proc_exit->exit_code == expected_status);
    test_assert(event->next_event == NULL);

    free_event_list(event);

    udi_error_e result = continue_process(proc);
    assert_no_error(result);

    result = free_process(proc);
    assert_no_error(result);
}
