//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

mod native_file_tests;
mod utils;

use udi::EventData;
use udi::ThreadState;

const NUM_THREADS: u8 = 10;

#[test]
fn thread() -> Result<(), udi::Error> {
    let metadata = native_file_tests::get_test_metadata();
    let binary_path = metadata.workerthreads_path().to_str().unwrap();
    let thread_break_addr = metadata.thread_break_addr();
    let start_notification_addr = metadata.start_notification_addr();
    let term_notification_addr = metadata.term_notification_addr();

    let config = udi::ProcessConfig::new(None, utils::rt_lib_path());
    let envp = Vec::new();
    let argv = vec![NUM_THREADS.to_string()];

    let proc_ref = udi::create_process(binary_path, &argv, &envp, &config)?;

    let thr_ref;
    {
        let mut process = proc_ref.lock()?;

        thr_ref = process.get_initial_thread();

        process.create_breakpoint(thread_break_addr)?;
        process.install_breakpoint(thread_break_addr)?;
        process.create_breakpoint(start_notification_addr)?;
        process.install_breakpoint(start_notification_addr)?;
        process.create_breakpoint(term_notification_addr)?;
        process.install_breakpoint(term_notification_addr)?;
    }

    utils::validate_thread_state(&proc_ref, ThreadState::Running);

    proc_ref.lock()?.continue_process()?;

    let mut threads_created = 0;
    let mut start_received = false;

    // Wait for main thread to hit start_notification and suspend when it does
    // Wait for worker threads to be created
    utils::handle_proc_events(&proc_ref, |e| {
        match e.data {
            EventData::Breakpoint { addr } => {
                if addr == start_notification_addr {
                    start_received = true;
                    e.thread
                        .lock()
                        .unwrap()
                        .suspend()
                        .expect("Failed to suspend main thread");
                } else {
                    panic!("Received unexpected breakpoint event {:?}", e.data)
                }
            }
            EventData::ThreadCreate { .. } => {
                threads_created += 1;
            }
            _ => panic!("Unexpected event {:?}", e.data),
        }

        start_received && threads_created == NUM_THREADS
    });

    thr_ref.lock()?.resume()?;

    proc_ref.lock()?.continue_process()?;

    let mut thread_breaks_received = 0;
    let mut term_received = false;

    utils::handle_proc_events(&proc_ref, |e| {
        match e.data {
            EventData::Breakpoint { addr } => {
                if addr == term_notification_addr {
                    term_received = true;
                    e.thread
                        .lock()
                        .unwrap()
                        .suspend()
                        .expect("Failed to suspend main thread");
                } else if addr == thread_break_addr {
                    thread_breaks_received += 1;
                    e.thread
                        .lock()
                        .unwrap()
                        .suspend()
                        .expect("Failed to suspend worker thread");
                } else {
                    panic!("Unexpected breakpoint event {:?}", e.data);
                }
            }
            _ => panic!("Unexpected event {:?}", e.data),
        }

        term_received && thread_breaks_received == NUM_THREADS
    });

    utils::validate_thread_state(&proc_ref, ThreadState::Suspended);

    for thread in proc_ref.lock()?.threads() {
        thread.lock()?.resume()?;
    }

    proc_ref.lock()?.continue_process()?;

    let mut thread_deaths_received = 0;

    utils::handle_proc_events(&proc_ref, |e| {
        match e.data {
            EventData::ThreadDeath => {
                thread_deaths_received += 1;
            }
            _ => panic!("Unexpected event {:?}", e.data),
        }

        thread_deaths_received == NUM_THREADS
    });

    proc_ref.lock()?.continue_process()?;

    utils::wait_for_exit(&proc_ref, &thr_ref, 0);

    Ok(())
}
