//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

use std::{ffi::CString, mem::MaybeUninit, path::PathBuf};

use udi_c::*;

mod native_file_tests;

const NUM_THREADS: u32 = 10;
const NUM_THREADS_STR: &str = "10";

#[test]
fn thread() -> Result<(), anyhow::Error> {
    let metadata = native_file_tests::get_test_metadata();
    let binary_path = metadata.workerthreads_path().to_str().unwrap();
    let thread_break_addr = metadata.thread_break_addr();
    let start_notification_addr = metadata.start_notification_addr();
    let term_notification_addr = metadata.term_notification_addr();

    unsafe {
        let executable = to_c_string(binary_path);
        let rt_lib_path = rt_lib_path();
        let config = udi_proc_config {
            root_dir: std::ptr::null(),
            rt_lib_path: to_c_string(rt_lib_path.as_ref().unwrap()),
        };
        let envp: &[*const libc::c_char] = &[std::ptr::null()];
        let argv: &[*const libc::c_char] = &[to_c_string(NUM_THREADS_STR), std::ptr::null()];
        let mut process: *const udi_process = std::ptr::null();

        to_result(create_process(
            executable,
            argv.as_ptr(),
            envp.as_ptr(),
            &config as *const udi_proc_config,
            &mut process,
        ))?;

        let mut initial_thr: *const udi_thread = std::ptr::null();
        to_result(get_initial_thread(process, &mut initial_thr))?;

        to_result(create_breakpoint(process, thread_break_addr))?;
        to_result(install_breakpoint(process, thread_break_addr))?;
        to_result(create_breakpoint(process, start_notification_addr))?;
        to_result(install_breakpoint(process, start_notification_addr))?;
        to_result(create_breakpoint(process, term_notification_addr))?;
        to_result(install_breakpoint(process, term_notification_addr))?;

        validate_thread_state(process, initial_thr, true)?;

        to_result(continue_process(process))?;

        let mut threads_created = 0;
        let mut start_received = false;

        // Wait for main thread to hit start_notification and suspend when it does
        // Wait for worker threads to be created
        while !start_received || threads_created < NUM_THREADS {
            let mut event_list: *const udi_event = std::ptr::null();

            to_result(wait_for_events(&process, 1, &mut event_list))?;

            let mut event = event_list;
            loop {
                match &(*event).event_type {
                    udi_event_type_e::UDI_EVENT_BREAKPOINT => {
                        let data = (*event).event_data as *const udi_event_breakpoint;
                        let addr = (*data).breakpoint_addr;
                        if addr == start_notification_addr {
                            start_received = true;

                            to_result(suspend_thread((*event).thr))?;
                        }
                    }
                    udi_event_type_e::UDI_EVENT_THREAD_CREATE => {
                        threads_created += 1;
                    }
                    _ => panic!("Unexpected event type"),
                }

                if (*event).next_event.is_null() {
                    break;
                }
                event = (*event).next_event;
            }

            if !event_list.is_null() {
                free_event_list(event_list);
            }

            if !start_received || threads_created < NUM_THREADS {
                to_result(continue_process(process))?;
            }
        }

        to_result(resume_thread(initial_thr))?;
        to_result(continue_process(process))?;

        let mut thread_breaks_received = 0;
        let mut term_received = false;

        while !term_received || thread_breaks_received < NUM_THREADS {
            let mut event_list: *const udi_event = std::ptr::null();

            to_result(wait_for_events(&process, 1, &mut event_list))?;

            let mut event = event_list;
            loop {
                match (*event).event_type {
                    udi_event_type_e::UDI_EVENT_BREAKPOINT => {
                        let data = (*event).event_data as *const udi_event_breakpoint;
                        let addr = (*data).breakpoint_addr;
                        if addr == term_notification_addr {
                            term_received = true;

                            to_result(suspend_thread((*event).thr))?;
                        } else if addr == thread_break_addr {
                            thread_breaks_received += 1;

                            to_result(suspend_thread((*event).thr))?;
                        } else {
                            panic!("Unexpected breakpoint event");
                        }
                    }
                    _ => panic!("Unexpected event type"),
                }

                if (*event).next_event.is_null() {
                    break;
                }
                event = (*event).next_event;
            }

            if !event_list.is_null() {
                free_event_list(event_list);
            }

            if !term_received || thread_breaks_received < NUM_THREADS {
                to_result(continue_process(process))?;
            }
        }

        validate_thread_state(process, initial_thr, false)?;

        let mut thr = initial_thr;
        while !thr.is_null() {
            to_result(resume_thread(thr))?;

            let mut next_thr = std::ptr::null();
            to_result(get_next_thread(process, thr, &mut next_thr))?;
            thr = next_thr;
        }

        to_result(continue_process(process))?;

        let mut thread_deaths_received = 0;

        while thread_deaths_received < NUM_THREADS {
            let mut event_list: *const udi_event = std::ptr::null();

            to_result(wait_for_events(&process, 1, &mut event_list))?;

            let mut event = event_list;
            loop {
                match (*event).event_type {
                    udi_event_type_e::UDI_EVENT_THREAD_DEATH => {
                        thread_deaths_received += 1;
                    }
                    _ => panic!("Unexpected event type"),
                }

                if (*event).next_event.is_null() {
                    break;
                }
                event = (*event).next_event;
            }

            if !event_list.is_null() {
                free_event_list(event_list);
            }

            if thread_deaths_received < NUM_THREADS {
                to_result(continue_process(process))?;
            }
        }

        to_result(continue_process(process))?;

        // Wait for exit event
        let mut event: *const udi_event = std::ptr::null();
        to_result(wait_for_events(&process, 1, &mut event))?;
        assert!((*event).next_event.is_null());
        match (*event).event_type {
            udi_event_type_e::UDI_EVENT_PROCESS_EXIT => {
                let data = (*event).event_data as *const udi_event_process_exit;
                assert_eq!(0, (*data).exit_code);
            }
            _ => panic!("Unexpected event type"),
        }
        free_event_list(event);

        to_result(continue_process(process))?;

        // Wait for process cleanup event
        let mut event: *const udi_event = std::ptr::null();
        to_result(wait_for_events(&process, 1, &mut event))?;
        assert!((*event).next_event.is_null());
        match (*event).event_type {
            udi_event_type_e::UDI_EVENT_PROCESS_CLEANUP => {}
            _ => panic!("Unexpected event type"),
        }
        free_event_list(event);

        Ok(())
    }
}

unsafe fn to_c_string(msg: &str) -> *const libc::c_char {
    let cstr = match CString::new(msg) {
        Ok(val) => val,
        Err(_) => return std::ptr::null(),
    };

    let len = cstr.to_bytes_with_nul().len();
    let output = libc::malloc(len);
    if !output.is_null() {
        libc::memcpy(output, cstr.as_ptr() as *const libc::c_void, len);
    }

    output as *const libc::c_char
}

unsafe fn to_result(result: udi_error) -> Result<(), anyhow::Error> {
    match result.code {
        udi_error_e::UDI_ERROR_NONE => Ok(()),
        udi_error_e::UDI_ERROR_NOMEM => Err(anyhow::anyhow!("Out of memory")),
        _ => {
            let msg = if result.msg.is_null() {
                "Unknown error".to_owned()
            } else {
                let msg = std::ffi::CStr::from_ptr(result.msg).to_str()?;
                let msg_str = msg.to_owned();
                libc::free(result.msg as *mut libc::c_void);
                msg_str
            };

            panic!("{}", msg);
            // Err(anyhow::anyhow!("{}", msg))
        }
    }
}

fn rt_lib_path() -> Option<String> {
    let mut path = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    path.push("..");
    path.push("libudirt");
    path.push("build");
    path.push("src");
    path.push(sys::get_rt_lib_name());

    Some(path.to_str().unwrap().to_owned())
}

unsafe fn validate_thread_state(
    process: *const udi_process,
    initial_thr: *const udi_thread,
    running: bool,
) -> Result<(), anyhow::Error> {
    to_result(refresh_state(process))?;

    let mut thr = initial_thr;
    while !thr.is_null() {
        let mut state = MaybeUninit::uninit();
        to_result(get_state(thr, state.as_mut_ptr()))?;

        match state.assume_init() {
            udi_thread_state_e::UDI_TS_RUNNING => {
                if !running {
                    return Err(anyhow::anyhow!("Thread is running"));
                }
            }
            udi_thread_state_e::UDI_TS_SUSPENDED => {
                if running {
                    return Err(anyhow::anyhow!("Thread is suspended"));
                }
            }
        }

        let mut next_thr = std::ptr::null();
        to_result(get_next_thread(process, thr, &mut next_thr))?;
        thr = next_thr;
    }

    Ok(())
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
