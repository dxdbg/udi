//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]
#![allow(dead_code)]

use ::std::sync::{Mutex, Arc};

use ::std::collections::HashSet;

use ::std::path::PathBuf;

use udi::Process;
use udi::Thread;
use udi::ThreadState;
use udi::EventData;
use udi::wait_for_events;
use udi::Event;
use udi::Error;

pub fn rt_lib_path() -> Option<String> {
    let mut path = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    path.push("..");
    path.push("libudirt");
    path.push("build");
    path.push("src");
    path.push(sys::get_rt_lib_name());

    Some(path.to_str().unwrap().to_owned())
}

pub fn print_error(e: Error) {
    println!("test failed: {}", e);

    for e in e.iter().skip(1) {
        println!("caused by: {}", e);
    }

    if let Some(backtrace) = e.backtrace() {
        println!("backtrace: {:?}", backtrace);
    }
}

pub fn wait_for_exit(process: &Arc<Mutex<Process>>,
                     thread: &Arc<Mutex<Thread>>,
                     expected_status: i32) {

    wait_for_event(process, thread, &EventData::ProcessExit{ code: expected_status });

    process.lock().expect("Failed unlock process").continue_process().expect("Continue failed");

    wait_for_event(process, thread, &EventData::ProcessCleanup);
}

pub fn wait_for_event(process: &Arc<Mutex<Process>>,
                      thread: &Arc<Mutex<Thread>>,
                      expected_event: &EventData) {

    let procs = vec![ process.clone() ];

    let events = wait_for_events(&procs).expect("Failed to wait for events");
    assert_eq!(1, events.len());

    let event = &events[0];
    assert_eq!(*expected_event, event.data);
    assert_proc_eq(&process, &event.process);
    assert_thr_eq(&thread, &event.thread);
}

pub fn handle_proc_events<F>(process: &Arc<Mutex<Process>>,
                             callback: F) where F: FnMut(&Event) -> bool {
    let procs = vec![ process.clone() ];

    handle_events(&procs, callback);
}

pub fn handle_events<F>(procs: &Vec<Arc<Mutex<Process>>>,
                        mut callback: F) where F: FnMut(&Event) -> bool {

    let mut complete = false;
    while !complete {
        let events = wait_for_events(procs).expect("Failed to wait for events");

        let mut event_procs = HashSet::new();
        let mut event_iter = events.iter();
        while !complete {
            match event_iter.next() {
                Some(event) => {
                    event_procs.insert(event.process.lock().unwrap().get_pid());
                    complete = callback(event);
                },
                None => {
                    break;
                }
            }
        }

        if let Some(_) = event_iter.next() {
            panic!("Callback did not consume all possible events");
        }

        if !complete {
            for proc_ref in procs.iter() {
                let mut process = proc_ref.lock().unwrap();
                if event_procs.contains(&process.get_pid()) {
                    process.continue_process().expect("Failed to continue process");
                }
            }
        }
    }
}

pub fn validate_thread_state(proc_ref: &Arc<Mutex<Process>>, state: ThreadState) {
    let mut process = proc_ref.lock().unwrap();
    process.refresh_state().expect("Failed to refresh state");

    for thr in process.threads() {
        let thr_state = thr.lock().unwrap().get_state();
        assert_eq!(state, thr_state);
    }
}

pub fn assert_proc_eq(lhs: &Arc<Mutex<Process>>, rhs: &Arc<Mutex<Process>>) {
    let lhs_pid = lhs.lock().expect("Failed to unlock lhs process").get_pid();
    let rhs_pid = rhs.lock().expect("Failed to unlock rhs process").get_pid();
    assert_eq!(lhs_pid, rhs_pid);
}

pub fn assert_thr_eq(lhs: &Arc<Mutex<Thread>>, rhs: &Arc<Mutex<Thread>>) {
    let lhs_tid = lhs.lock().expect("Failed to unlock lhs thread").get_tid();
    let rhs_tid = rhs.lock().expect("Failed to unlock rhs thread").get_tid();
    assert_eq!(lhs_tid, rhs_tid);
}

#[cfg(target_os = "linux")]
mod sys {
    pub fn get_rt_lib_name() -> &'static str {
        "libudirt.so"
    }
}

#[cfg(target_os = "macos")]
mod sys {
    pub fn get_rt_lib_name() -> &'static str {
        "libudirt.dylib"
    }
}

#[cfg(target_os = "windows")]
mod sys {
    pub fn get_rt_lib_name() -> &'static str {
        "udirt.dll"
    }
}
