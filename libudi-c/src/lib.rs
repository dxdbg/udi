//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]
#![allow(non_camel_case_types)]
#![allow(clippy::missing_safety_doc, clippy::undocumented_unsafe_blocks)]

use std::ffi::{CStr, CString};
use std::mem::{size_of, transmute};
use std::sync::{Arc, Mutex};

use udi::{Error, EventData, Process, ProcessConfig, Register, Thread, UserData};

/// Opaque thread handle
pub struct udi_thread {
    handle: Arc<Mutex<Thread>>,
    tid: u64,
    next: *const udi_thread,
}

/// Opaque process handle
pub struct udi_process {
    handle: Arc<Mutex<Process>>,
    pid: u32,
    thr: *const udi_thread,
}

/// Library error codes
#[repr(u32)]
pub enum udi_error_e {
    /// On internal, unrecoverable library error.
    UDI_ERROR_LIBRARY = 0,

    /// On invalid request.
    UDI_ERROR_REQUEST = 1,

    /// On memory allocation error.
    UDI_ERROR_NOMEM = 2,

    /// On success.
    UDI_ERROR_NONE = 3,
}

/// Error structure
#[repr(C)]
pub struct udi_error {
    /// The error code
    pub code: udi_error_e,

    /// A heap allocated (via malloc) error string. It will be null if memory
    /// could not be allocated for the error message. Callers of UDI functions
    /// are expected to free this value.
    pub msg: *const libc::c_char,
}

/// Architecture of a debuggee
#[repr(u16)]
pub enum udi_arch_e {
    UDI_ARCH_X86 = 0,
    UDI_ARCH_X86_64 = 1,
}

/// The running state of a thread
#[repr(u8)]
pub enum udi_thread_state_e {
    UDI_TS_RUNNING = 0,
    UDI_TS_SUSPENDED = 1,
}

/// Register identifiers
#[repr(u32)]
pub enum udi_register_e {
    // X86 registers
    UDI_X86_MIN = 0,
    UDI_X86_GS = 1,
    UDI_X86_FS,
    UDI_X86_ES,
    UDI_X86_DS,
    UDI_X86_EDI,
    UDI_X86_ESI,
    UDI_X86_EBP,
    UDI_X86_ESP,
    UDI_X86_EBX,
    UDI_X86_EDX,
    UDI_X86_ECX,
    UDI_X86_EAX,
    UDI_X86_CS,
    UDI_X86_SS,
    UDI_X86_EIP,
    UDI_X86_FLAGS,
    UDI_X86_ST0,
    UDI_X86_ST1,
    UDI_X86_ST2,
    UDI_X86_ST3,
    UDI_X86_ST4,
    UDI_X86_ST5,
    UDI_X86_ST6,
    UDI_X86_ST7,
    UDI_X86_MAX,

    // X86_64 registers
    UDI_X86_64_MIN,
    UDI_X86_64_R8,
    UDI_X86_64_R9,
    UDI_X86_64_R10,
    UDI_X86_64_R11,
    UDI_X86_64_R12,
    UDI_X86_64_R13,
    UDI_X86_64_R14,
    UDI_X86_64_R15,
    UDI_X86_64_RDI,
    UDI_X86_64_RSI,
    UDI_X86_64_RBP,
    UDI_X86_64_RBX,
    UDI_X86_64_RDX,
    UDI_X86_64_RAX,
    UDI_X86_64_RCX,
    UDI_X86_64_RSP,
    UDI_X86_64_RIP,
    UDI_X86_64_CSGSFS,
    UDI_X86_64_FLAGS,
    UDI_X86_64_ST0,
    UDI_X86_64_ST1,
    UDI_X86_64_ST2,
    UDI_X86_64_ST3,
    UDI_X86_64_ST4,
    UDI_X86_64_ST5,
    UDI_X86_64_ST6,
    UDI_X86_64_ST7,
    UDI_X86_64_XMM0,
    UDI_X86_64_XMM1,
    UDI_X86_64_XMM2,
    UDI_X86_64_XMM3,
    UDI_X86_64_XMM4,
    UDI_X86_64_XMM5,
    UDI_X86_64_XMM6,
    UDI_X86_64_XMM7,
    UDI_X86_64_XMM8,
    UDI_X86_64_XMM9,
    UDI_X86_64_XMM10,
    UDI_X86_64_XMM11,
    UDI_X86_64_XMM12,
    UDI_X86_64_XMM13,
    UDI_X86_64_XMM14,
    UDI_X86_64_XMM15,
    UDI_X86_64_MAX,
}

trait UnsafeFrom<T> {
    unsafe fn from(_: T) -> Self;
}

impl UnsafeFrom<Result<(), udi::Error>> for udi_error {
    unsafe fn from(result: Result<(), udi::Error>) -> udi_error {
        match result {
            Ok(_) => udi_error {
                code: udi_error_e::UDI_ERROR_NONE,
                msg: std::ptr::null(),
            },
            Err(e) => UnsafeFrom::from(e),
        }
    }
}

impl UnsafeFrom<Error> for udi_error {
    unsafe fn from(e: Error) -> udi_error {
        match e {
            Error::Request(ref msg) => udi_error {
                code: udi_error_e::UDI_ERROR_REQUEST,
                msg: to_c_string(msg),
            },
            _ => {
                let msg = format!("{}", e);
                udi_error {
                    code: udi_error_e::UDI_ERROR_LIBRARY,
                    msg: to_c_string(&msg),
                }
            }
        }
    }
}

impl UnsafeFrom<udi_error> for udi_error {
    unsafe fn from(e: udi_error) -> udi_error {
        e
    }
}

impl<'a> UnsafeFrom<std::sync::PoisonError<std::sync::MutexGuard<'a, Process>>> for udi_error {
    unsafe fn from(_: std::sync::PoisonError<std::sync::MutexGuard<'a, Process>>) -> udi_error {
        udi_error {
            code: udi_error_e::UDI_ERROR_LIBRARY,
            msg: to_c_string("lock failed"),
        }
    }
}

impl<'a> UnsafeFrom<std::sync::PoisonError<std::sync::MutexGuard<'a, Thread>>> for udi_error {
    unsafe fn from(_: std::sync::PoisonError<std::sync::MutexGuard<'a, Thread>>) -> udi_error {
        udi_error {
            code: udi_error_e::UDI_ERROR_LIBRARY,
            msg: to_c_string("lock failed"),
        }
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

macro_rules! try_err {
    ($e:expr) => {
        match $e {
            Ok(val) => val,
            Err(err) => return UnsafeFrom::from(err),
        }
    };
}

macro_rules! try_unsafe {
    ($e:expr) => {
        match $e {
            Ok(val) => val,
            Err(err) => return Err(UnsafeFrom::from(err)),
        }
    };
}

#[repr(C)]
pub struct udi_proc_config {
    pub root_dir: *const libc::c_char,
    pub rt_lib_path: *const libc::c_char,
}

/// Create UDI-controlled process. This function is very similar to the UNIX execve
/// function/system call.
///
/// # Arguments
///
/// * `executable` - the full path to the executable
/// * `argv` - the arguments
/// * `envp` - the environment, if NULL, the newly created process will inherit the
///            environment for this process
/// * `config` - the configuration for creating the process
/// * `process` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn create_process(
    executable: *const libc::c_char,
    argv: *const *const libc::c_char,
    envp: *const *const libc::c_char,
    config: *const udi_proc_config,
    process: *mut *const udi_process,
) -> udi_error {
    if executable.is_null() {
        *process = std::ptr::null();
        return udi_error {
            code: udi_error_e::UDI_ERROR_REQUEST,
            msg: to_c_string("Executable cannot be null"),
        };
    }

    if argv.is_null() {
        *process = std::ptr::null();
        return udi_error {
            code: udi_error_e::UDI_ERROR_REQUEST,
            msg: to_c_string("Argument array cannot be null"),
        };
    }

    if config.is_null() {
        *process = std::ptr::null();
        return udi_error {
            code: udi_error_e::UDI_ERROR_REQUEST,
            msg: to_c_string("Process config cannot be null"),
        };
    }

    let exec_str = match CStr::from_ptr(executable).to_str() {
        Ok(val) => val,
        Err(_) => {
            *process = std::ptr::null();
            return udi_error {
                code: udi_error_e::UDI_ERROR_REQUEST,
                msg: to_c_string("Executable is not a valid UTF-8 string"),
            };
        }
    };

    let argv_vec = match to_vec(argv) {
        Ok(val) => val,
        Err(msg) => {
            *process = std::ptr::null();
            return udi_error {
                code: udi_error_e::UDI_ERROR_REQUEST,
                msg,
            };
        }
    };

    let envp_vec = if !envp.is_null() {
        match to_vec(envp) {
            Ok(val) => val,
            Err(msg) => {
                *process = std::ptr::null();
                return udi_error {
                    code: udi_error_e::UDI_ERROR_REQUEST,
                    msg,
                };
            }
        }
    } else {
        vec![]
    };

    let root_dir_str = if !((*config).root_dir.is_null()) {
        match CStr::from_ptr((*config).root_dir).to_str() {
            Ok(val) => Some(val.to_owned()),
            Err(_) => {
                *process = std::ptr::null();
                return udi_error {
                    code: udi_error_e::UDI_ERROR_REQUEST,
                    msg: to_c_string("Process config root dir is not a valid UTF-8 string"),
                };
            }
        }
    } else {
        None
    };

    let rt_lib_path_str = if !((*config).rt_lib_path.is_null()) {
        match CStr::from_ptr((*config).rt_lib_path).to_str() {
            Ok(val) => Some(val.to_owned()),
            Err(_) => {
                *process = std::ptr::null();
                return udi_error {
                    code: udi_error_e::UDI_ERROR_REQUEST,
                    msg: to_c_string("Process config rt lib path is not a valid UTF-8 string"),
                };
            }
        }
    } else {
        None
    };

    let proc_config = ProcessConfig::new(root_dir_str, rt_lib_path_str);

    match udi::create_process(exec_str, &argv_vec, &envp_vec, &proc_config) {
        Ok(p) => {
            let thread_ptr;
            let pid;
            {
                let udi_process = try_err!(p.lock());
                pid = udi_process.get_pid();

                let thr_handle = udi_process.get_initial_thread();
                let tid = try_err!(thr_handle.lock()).get_tid();

                thread_ptr = Box::new(udi_thread {
                    handle: udi_process.get_initial_thread(),
                    tid,
                    next: std::ptr::null(),
                });
            }

            let process_ptr = Box::new(udi_process {
                handle: p,
                pid,
                thr: Box::into_raw(thread_ptr),
            });
            *process = Box::into_raw(process_ptr);

            udi_error {
                code: udi_error_e::UDI_ERROR_NONE,
                msg: std::ptr::null(),
            }
        }
        Err(e) => {
            *process = std::ptr::null();

            UnsafeFrom::from(e)
        }
    }
}

unsafe fn to_vec(
    arr: *const *const libc::c_char,
) -> std::result::Result<Vec<String>, *const libc::c_char> {
    let mut x = 0;

    let mut output: Vec<String> = vec![];
    while !((*arr.offset(x)).is_null()) {
        let elem = *arr.offset(x);

        let elem_str = match CStr::from_ptr(elem).to_str() {
            Ok(val) => val.to_owned(),
            Err(_) => return Err(to_c_string("Invalid string in specified array")),
        };

        output.push(elem_str);

        x += 1;
    }

    Ok(output)
}

/// Tells the library that resources allocated for the process can be released.
///
/// # Arguments
///
/// * `process` - the process to free
#[no_mangle]
pub unsafe extern "C" fn free_process(process: *const udi_process) {
    let mut thr_ptr = (*process).thr;
    while !thr_ptr.is_null() {
        let next_ptr = (*thr_ptr).next;

        drop(Box::from_raw(thr_ptr as *mut udi_thread));

        thr_ptr = next_ptr;
    }

    let process_ptr = Box::from_raw(process as *mut udi_process);
    drop(process_ptr);
}

/// Continue a stopped UDI process.
///
/// # Arguments
///
/// * `process` - the process to continue
#[no_mangle]
pub unsafe extern "C" fn continue_process(process: *const udi_process) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.continue_process())
}

/// Refresh the state of the specified process.
///
/// # Arguments
///
/// * `process` - the process to refresh
#[no_mangle]
pub unsafe extern "C" fn refresh_state(process: *const udi_process) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.refresh_state())
}

#[derive(Debug)]
struct CUserData {
    pub ptr: *const libc::c_void,
}

impl UserData for CUserData {}

/// Sets the user data stored with the internal process structure.
///
/// # Arguments
///
/// * `process` - the process to set the user data for
/// * `data` - the user data to set
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn set_user_data(
    process: *const udi_process,
    data: *const libc::c_void,
) -> udi_error {
    let data_ptr = Box::new(CUserData { ptr: data });

    let mut process = try_err!((*process).handle.lock());

    process.set_user_data(data_ptr);

    UnsafeFrom::from(Ok(()))
}

/// Gets the use data stored with the internal process structure.
///
/// # Arguments
///
/// * `process` - the process to get the user data for
/// * `data` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_user_data(
    process: *const udi_process,
    data: *mut *const libc::c_void,
) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    let user_data = process
        .get_user_data()
        .and_then(|d| d.downcast_ref::<CUserData>())
        .map(|c| c.ptr)
        .unwrap_or(std::ptr::null());

    *data = user_data;

    UnsafeFrom::from(Ok(()))
}

/// Gets the process identifier for the specified process.
///
/// # Arguments
///
/// * `process` - the process to get the identifier for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_proc_pid(process: *const udi_process, output: *mut u32) -> udi_error {
    let process = try_err!((*process).handle.lock());

    *output = process.get_pid();

    UnsafeFrom::from(Ok(()))
}

/// Gets the architecture for the specified process.
///
/// # Arguments
///
/// * `process` - the process to get the architecture for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_proc_architecture(
    process: *const udi_process,
    output: *mut udi_arch_e,
) -> udi_error {
    let process = try_err!((*process).handle.lock());

    *output = match process.get_architecture() {
        udi::Architecture::X86 => udi_arch_e::UDI_ARCH_X86,
        udi::Architecture::X86_64 => udi_arch_e::UDI_ARCH_X86_64,
    };

    UnsafeFrom::from(Ok(()))
}

/// Gets whether the specified process is multithread capable.
///
/// # Arguments
///
/// * `process` - the process to get the multithread capable flag for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_multithread_capable(
    process: *const udi_process,
    output: *mut i32,
) -> udi_error {
    let process = try_err!((*process).handle.lock());

    *output = process.is_multithread_capable() as i32;

    UnsafeFrom::from(Ok(()))
}

/// Gets the initial thread in the specified process.
///
/// # Arguments
///
/// * `process` - the process to get the initial thread for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_initial_thread(
    process: *const udi_process,
    output: *mut *const udi_thread,
) -> udi_error {
    let internal_process = try_err!((*process).handle.lock());

    *output = (*process).thr;

    drop(internal_process);

    UnsafeFrom::from(Ok(()))
}

/// Checks if the process is in a running state.
///
/// # Arguments
///
/// * `process` - the process to check
/// * `output` - non-zero if the process has been continued, but events haven't been received yet
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn is_running(process: *const udi_process, output: *mut i32) -> udi_error {
    let process = try_err!((*process).handle.lock());

    *output = process.is_running() as i32;

    UnsafeFrom::from(Ok(()))
}

/// Checks if the process is in a terminated state.
///
/// # Arguments
///
/// * `process` - the process to check
/// * `output` - non-zero if the process has been terminated and can no longer be interacted with
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn is_terminated(process: *const udi_process, output: *mut i32) -> udi_error {
    let process = try_err!((*process).handle.lock());

    *output = process.is_terminated() as i32;

    UnsafeFrom::from(Ok(()))
}

/// Sets the user data stored with the internal thread structure.
///
/// # Arguments
///
/// * `thr` - the thread to set the user data for
/// * `data` - the user data to set
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn set_thread_user_data(
    thr: *const udi_thread,
    data: *const libc::c_void,
) -> udi_error {
    let user_data = Box::new(CUserData { ptr: data });

    let mut thr = try_err!((*thr).handle.lock());

    thr.set_user_data(user_data);

    UnsafeFrom::from(Ok(()))
}

/// Gets the user data stored with the internal thread structure.
///
/// # Arguments
///
/// * `thr` - the thread to get the user data for
/// * `data` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_thread_user_data(
    thr: *const udi_thread,
    data: *mut *const libc::c_void,
) -> udi_error {
    let mut thr = try_err!((*thr).handle.lock());

    let user_data = thr
        .get_user_data()
        .and_then(|d| d.downcast_ref::<CUserData>())
        .map(|c| c.ptr)
        .unwrap_or(std::ptr::null());

    *data = user_data;

    UnsafeFrom::from(Ok(()))
}

/// Gets the thread id for the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to get the thread id for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_tid(thr: *const udi_thread, output: *mut u64) -> udi_error {
    let thr = try_err!((*thr).handle.lock());

    *output = thr.get_tid();

    UnsafeFrom::from(Ok(()))
}

/// Gets the state for the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to get the state for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_state(
    thr: *const udi_thread,
    output: *mut udi_thread_state_e,
) -> udi_error {
    let thr = try_err!((*thr).handle.lock());

    *output = match thr.get_state() {
        udi::ThreadState::Running => udi_thread_state_e::UDI_TS_RUNNING,
        udi::ThreadState::Suspended => udi_thread_state_e::UDI_TS_SUSPENDED,
    };

    UnsafeFrom::from(Ok(()))
}

/// Gets the next thread.
///
/// # Arguments
///
/// * `proc` - the process to get the next thread for
/// * `thr` - the current thread
/// * `output` - populated on success or NULL if `thr` is the last thread
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_next_thread(
    proc_wrapper: *const udi_process,
    thr: *const udi_thread,
    output: *mut *const udi_thread,
) -> udi_error {
    let process = try_err!((*proc_wrapper).handle.lock());

    *output = (*thr).next;

    drop(process);

    UnsafeFrom::from(Ok(()))
}

/// Resumes the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to resume
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn resume_thread(thr: *const udi_thread) -> udi_error {
    let mut thr = try_err!((*thr).handle.lock());

    UnsafeFrom::from(thr.resume())
}

/// Suspends the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to suspend
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn suspend_thread(thr: *const udi_thread) -> udi_error {
    let mut thr = try_err!((*thr).handle.lock());

    UnsafeFrom::from(thr.suspend())
}

/// Sets the single step setting for a specific thread.
///
/// # Arguments
///
/// * `thr` - the thread to set the single step setting for
/// * `enable` - non-zero to enable single stepping, zero to disable
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn set_single_step(thr: *const udi_thread, enable: libc::c_int) -> udi_error {
    let mut thr = try_err!((*thr).handle.lock());

    UnsafeFrom::from(thr.set_single_step(enable != 0))
}

/// Gets the current single step setting for a specific thread.
///
/// # Arguments
///
/// * `thr` - the thread to get the single step setting for
/// * `output` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_single_step(thr: *const udi_thread, output: *mut i32) -> udi_error {
    let thr = try_err!((*thr).handle.lock());

    *output = thr.get_single_step() as i32;

    UnsafeFrom::from(Ok(()))
}

/// Creates a breakpoint in the specified process at the specified virtual address.
///
/// # Arguments
///
/// * `process` - the process to create the breakpoint in
/// * `addr` - the virtual address for the breakpoint
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn create_breakpoint(process: *const udi_process, addr: u64) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.create_breakpoint(addr))
}

/// Install a previously created breakpoint into the specified process' memory.
///
/// # Arguments
///
/// * `process` - the process to install the breakpoint into
/// * `addr` - the virtual address for the breakpoint
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn install_breakpoint(process: *const udi_process, addr: u64) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.install_breakpoint(addr))
}

/// Remove a previously installed breakpoint from the specified process' memory.
///
/// # Arguments
///
/// * `process` - the process to remove the breakpoint from
/// * `addr` - the virtual address for the breakpoint
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn remove_breakpoint(process: *const udi_process, addr: u64) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.remove_breakpoint(addr))
}

/// Delete a previously created breakpoint for the specified process.
///
/// # Arguments
///
/// * `process` - the process to delete the breakpoint from
/// * `addr` - the virtual address for the breakpoint
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn delete_breakpoint(process: *const udi_process, addr: u64) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    UnsafeFrom::from(process.delete_breakpoint(addr))
}

/// Read memory from the specified process.
///
/// # Arguments
///
/// * `process` - the process to read memory from
/// * `dst` - the destination for the memory read from the process
/// * `size` - the size of the memory to read
/// * `addr` - the virtual address to read from
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn read_mem(
    process: *const udi_process,
    dst: *mut u8,
    size: u32,
    addr: u64,
) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    let data = try_err!(process.read_mem(size, addr));
    let src = &data[0] as *const u8;
    libc::memcpy(
        dst as *mut libc::c_void,
        src as *const libc::c_void,
        size as usize,
    );
    UnsafeFrom::from(Ok(()))
}

/// Write memory in the specified process.
///
/// # Arguments
///
/// * `process` - the process to write memory in
/// * `src` - the source for the memory to write to the process
/// * `size` - the size of the memory to write
/// * `addr` - the virtual address to write to
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn write_mem(
    process: *const udi_process,
    src: *const u8,
    size: u32,
    addr: u64,
) -> udi_error {
    let mut process = try_err!((*process).handle.lock());

    let src = std::slice::from_raw_parts(src, size as usize);
    UnsafeFrom::from(process.write_mem(src, addr))
}

/// Read register from the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to read the register from
/// * `reg` - the register to read
/// * `value` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn read_register(
    thr: *const udi_thread,
    reg: udi_register_e,
    value: *mut u64,
) -> udi_error {
    let mut thread = try_err!((*thr).handle.lock());

    let register = transmute::<udi_register_e, Register>(reg);

    *value = try_err!(thread.read_register(register));

    UnsafeFrom::from(Ok(()))
}

/// Write register in the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to write the register in
/// * `reg` - the register to write
/// * `value` - the value to write
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn write_register(
    thr: *const udi_thread,
    reg: udi_register_e,
    value: u64,
) -> udi_error {
    let mut thread = try_err!((*thr).handle.lock());

    let register = transmute::<udi_register_e, Register>(reg);

    UnsafeFrom::from(thread.write_register(register, value))
}

/// Get the program counter for the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to get the program counter for
/// * `pc` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_pc(thr: *const udi_thread, pc: *mut u64) -> udi_error {
    let mut thread = try_err!((*thr).handle.lock());

    *pc = try_err!(thread.get_pc());

    UnsafeFrom::from(Ok(()))
}

/// Get the next instruction for the specified thread.
///
/// # Arguments
///
/// * `thr` - the thread to get the next instruction for
/// * `instr` - populated on success
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn get_next_instruction(
    thr: *const udi_thread,
    instr: *mut u64,
) -> udi_error {
    let mut thread = try_err!((*thr).handle.lock());

    *instr = try_err!(thread.get_next_instruction());

    UnsafeFrom::from(Ok(()))
}

#[repr(u32)]
pub enum udi_event_type_e {
    UDI_EVENT_UNKNOWN = 0,
    UDI_EVENT_ERROR = 1,
    UDI_EVENT_SIGNAL,
    UDI_EVENT_BREAKPOINT,
    UDI_EVENT_THREAD_CREATE,
    UDI_EVENT_THREAD_DEATH,
    UDI_EVENT_PROCESS_EXIT,
    UDI_EVENT_PROCESS_FORK,
    UDI_EVENT_PROCESS_EXEC,
    UDI_EVENT_SINGLE_STEP,
    UDI_EVENT_PROCESS_CLEANUP,
}

/// Represents an event that occurred in a debuggee.
#[repr(C)]
pub struct udi_event {
    pub event_type: udi_event_type_e,
    pub process: *const udi_process,
    pub thr: *const udi_thread,
    pub event_data: *const libc::c_void,
    pub next_event: *const udi_event,
}

/// When udi_event.event_type == UDI_EVENT_ERROR,
/// typeof(udi_event.event_data) == udi_event_error
#[repr(C)]
pub struct udi_event_error {
    pub errstr: *const libc::c_char,
}

/// When udi_event.event_type == UDI_EVENT_PROCESS_EXIT
/// typeof(udi_event.event_data) == udi_event_process_exit
#[repr(C)]
pub struct udi_event_process_exit {
    pub exit_code: i32,
}

/// When udi_event.event_type == UDI_EVENT_PROCESS_FORK
/// typeof(udi_event.event_data) == udi_event_process_fork
#[repr(C)]
pub struct udi_event_process_fork {
    pub pid: u32,
}

/// When udi_event.event_type == UDI_EVENT_PROCESS_EXEC
/// typeof(udi_event.event_data) == udi_event_process_exec
#[repr(C)]
pub struct udi_event_process_exec {
    pub path: *const libc::c_char,
    pub argv: *const *const libc::c_char,
    pub envp: *const *const libc::c_char,
}

/// When udi_event.event_type == UDI_EVENT_BREAKPOINT
/// typeof(udi_event.event_data) == udi_event_breakpoint
#[repr(C)]
pub struct udi_event_breakpoint {
    pub breakpoint_addr: u64,
}

/// When udi_event.event_type == UDI_EVENT_THREAD_CREATE
/// typeof(udi_event.event_data) == udi_event_thread_create
#[repr(C)]
pub struct udi_event_thread_create {
    pub new_thr: *const udi_thread,
}

/// When udi_event.event_type == UDI_EVENT_SIGNAL
/// typeof(udi_event.event_data) == udi_event_signal
#[repr(C)]
pub struct udi_event_signal {
    pub addr: u64,
    pub sig: u32,
}

/// Wait for events to occur in the specified processes.
///
/// # Arguments
///
/// * `procs` - the processes to wait for events in
/// * `num_procs` - the number of processes in the `procs` array
/// * `output` - populated on success. NULL if no events were received.
///
/// # Returns
///
/// The result of the operation.
#[no_mangle]
pub unsafe extern "C" fn wait_for_events(
    procs: *const *const udi_process,
    num_procs: libc::c_int,
    output: *mut *const udi_event,
) -> udi_error {
    let procs_slice: &[*const udi_process] = std::slice::from_raw_parts(procs, num_procs as usize);

    let procs_vec = procs_slice
        .iter()
        .map(|p| (**p).handle.clone())
        .collect::<Vec<Arc<Mutex<Process>>>>();

    let events = try_err!(udi::wait_for_events(&procs_vec));

    let mut output_events;
    if !events.is_empty() {
        output_events = std::ptr::null();
        let mut last_event = std::ptr::null();
        for event in events {
            let proc_handle = match find_proc_handle(procs_slice, &event.process) {
                Ok(p) => p,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

            match handle_thread_create(proc_handle, &event) {
                Ok(()) => {}
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            }

            let thr_handle = match find_thr_handle(proc_handle, &event.thread) {
                Ok(t) => t,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

            let event_struct = match to_event_struct(proc_handle, thr_handle, &event.data) {
                Ok(val) => val,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

            let current_event = match try_malloc(size_of::<udi_event>()) {
                Ok(p) => p as *mut udi_event,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

            *current_event = event_struct;
            (*current_event).next_event = last_event;
            last_event = current_event;

            if output_events.is_null() {
                output_events = current_event;
            }
        }
    } else {
        output_events = std::ptr::null();
    }

    *output = output_events;

    UnsafeFrom::from(Ok(()))
}

unsafe fn handle_thread_create(
    process: *const udi_process,
    event: &udi::Event,
) -> Result<(), udi_error> {
    match event.data {
        udi::EventData::ThreadCreate { tid } => {
            let internal_process = try_unsafe!(event.process.lock());

            for handle in internal_process.threads() {
                let internal_thread = try_unsafe!(handle.lock());
                if internal_thread.get_tid() == tid {
                    let mut thr: *mut udi_thread = (*process).thr as *mut udi_thread;

                    while !thr.is_null() {
                        if (*thr).next.is_null() {
                            break;
                        }
                        thr = (*thr).next as *mut udi_thread;
                    }

                    let new_thr = Box::new(udi_thread {
                        handle: handle.clone(),
                        tid,
                        next: std::ptr::null(),
                    });
                    (*thr).next = Box::into_raw(new_thr);

                    return Ok(());
                }
                drop(internal_thread);
            }

            return Err(udi_error {
                code: udi_error_e::UDI_ERROR_LIBRARY,
                msg: to_c_string("Could not locate event thread handle for newly created thread"),
            });
        }
        _ => {}
    }

    Ok(())
}

unsafe fn find_proc_handle(
    procs_slice: &[*const udi_process],
    process: &Arc<Mutex<Process>>,
) -> Result<*const udi_process, udi_error> {
    let pid = try_unsafe!(process.lock()).get_pid();

    let opt = procs_slice.iter().find(|&&p| (*p).pid == pid).copied();

    opt.ok_or_else(|| udi_error {
        code: udi_error_e::UDI_ERROR_LIBRARY,
        msg: to_c_string("Could not locate event process handle"),
    })
}

unsafe fn find_thr_handle(
    proc_handle: *const udi_process,
    thread: &Arc<Mutex<Thread>>,
) -> Result<*const udi_thread, udi_error> {
    let tid = try_unsafe!(thread.lock()).get_tid();

    find_thr_handle_for_tid(proc_handle, tid)
}
unsafe fn find_thr_handle_for_tid(
    proc_handle: *const udi_process,
    tid: u64,
) -> Result<*const udi_thread, udi_error> {
    let mut thr_iter = (*proc_handle).thr;

    let mut opt = None;
    while !thr_iter.is_null() {
        if (*thr_iter).tid == tid {
            opt = Some(thr_iter);
            break;
        }

        thr_iter = (*thr_iter).next;
    }

    opt.ok_or_else(|| udi_error {
        code: udi_error_e::UDI_ERROR_LIBRARY,
        msg: to_c_string("Could not locate event thread handle"),
    })
}

unsafe fn to_event_struct(
    proc_handle: *const udi_process,
    thr_handle: *const udi_thread,
    event_data: &EventData,
) -> Result<udi_event, udi_error> {
    let (event_type, raw_event_data) = match *event_data {
        EventData::Error { ref msg } => {
            let error_struct = try_malloc(size_of::<udi_event_error>())? as *mut udi_event_error;

            (*error_struct).errstr = to_c_string(msg);

            (
                udi_event_type_e::UDI_EVENT_ERROR,
                error_struct as *const libc::c_void,
            )
        }
        EventData::Signal { addr, sig } => {
            let signal_struct = try_malloc(size_of::<udi_event_signal>())? as *mut udi_event_signal;

            (*signal_struct).addr = addr;
            (*signal_struct).sig = sig;

            (udi_event_type_e::UDI_EVENT_SIGNAL, std::ptr::null())
        }
        EventData::Breakpoint { addr } => {
            let brkpt_struct =
                try_malloc(size_of::<udi_event_breakpoint>())? as *mut udi_event_breakpoint;

            (*brkpt_struct).breakpoint_addr = addr;

            (
                udi_event_type_e::UDI_EVENT_BREAKPOINT,
                brkpt_struct as *const libc::c_void,
            )
        }
        EventData::ThreadCreate { tid } => {
            let thr_struct =
                try_malloc(size_of::<udi_event_thread_create>())? as *mut udi_event_thread_create;

            (*thr_struct).new_thr = find_thr_handle_for_tid(proc_handle, tid)?;

            (
                udi_event_type_e::UDI_EVENT_THREAD_CREATE,
                thr_struct as *const libc::c_void,
            )
        }
        EventData::ThreadDeath => (udi_event_type_e::UDI_EVENT_THREAD_DEATH, std::ptr::null()),
        EventData::ProcessExit { code } => {
            let exit_struct =
                try_malloc(size_of::<udi_event_process_exit>())? as *mut udi_event_process_exit;

            (*exit_struct).exit_code = code;

            (
                udi_event_type_e::UDI_EVENT_PROCESS_EXIT,
                exit_struct as *const libc::c_void,
            )
        }
        EventData::ProcessFork { pid } => {
            let fork_struct =
                try_malloc(size_of::<udi_event_process_fork>())? as *mut udi_event_process_fork;

            (*fork_struct).pid = pid;

            (
                udi_event_type_e::UDI_EVENT_PROCESS_FORK,
                fork_struct as *const libc::c_void,
            )
        }
        EventData::ProcessExec {
            ref path,
            ref argv,
            ref envp,
        } => {
            let exec_struct =
                try_malloc(size_of::<udi_event_process_exec>())? as *mut udi_event_process_exec;

            (*exec_struct).argv = match to_null_term_array(argv) {
                Ok(argv) => argv,
                Err(e) => {
                    libc::free(exec_struct as *mut libc::c_void);
                    return Err(e);
                }
            };

            (*exec_struct).envp = match to_null_term_array(envp) {
                Ok(envp) => envp,
                Err(e) => {
                    free_null_term_array((*exec_struct).argv);
                    libc::free(exec_struct as *mut libc::c_void);
                    return Err(e);
                }
            };

            (*exec_struct).path = to_c_string(path);
            if (*exec_struct).path.is_null() {
                free_null_term_array((*exec_struct).argv);
                free_null_term_array((*exec_struct).envp);
                libc::free(exec_struct as *mut libc::c_void);
                return Err(udi_error {
                    code: udi_error_e::UDI_ERROR_NOMEM,
                    msg: std::ptr::null(),
                });
            }

            (
                udi_event_type_e::UDI_EVENT_PROCESS_EXEC,
                exec_struct as *const libc::c_void,
            )
        }
        EventData::SingleStep => (udi_event_type_e::UDI_EVENT_SINGLE_STEP, std::ptr::null()),
        EventData::ProcessCleanup => (
            udi_event_type_e::UDI_EVENT_PROCESS_CLEANUP,
            std::ptr::null(),
        ),
    };

    Ok(udi_event {
        process: proc_handle,
        thr: thr_handle,
        event_type,
        event_data: raw_event_data,
        next_event: std::ptr::null(),
    })
}

unsafe fn to_null_term_array(input: &Vec<String>) -> Result<*const *const libc::c_char, udi_error> {
    let len = input.len();
    let output =
        try_malloc(size_of::<*const libc::c_char>() * (len + 1))? as *mut *const libc::c_char;
    *(output.add(len)) = std::ptr::null();

    for (i, elem) in input.iter().enumerate() {
        let elem_c_str = to_c_string(elem);
        if elem_c_str.is_null() {
            free_null_term_array(output);
            return Err(udi_error {
                code: udi_error_e::UDI_ERROR_NOMEM,
                msg: std::ptr::null(),
            });
        }
        *(output.add(i)) = elem_c_str;
    }

    Ok(output)
}

unsafe fn free_null_term_array(input: *const *const libc::c_char) {
    let mut index = 0;
    loop {
        let current = *(input.offset(index));
        if !current.is_null() {
            libc::free(current as *mut libc::c_void);
            index += 1;
        } else {
            break;
        }
    }

    libc::free(input as *mut libc::c_void);
}

/// Frees the event list returned by wait_for_events.
///
/// # Arguments
///
/// * `event_list` - the event list to free
#[no_mangle]
pub unsafe extern "C" fn free_event_list(event_list: *const udi_event) {
    let mut iter = event_list;

    while !iter.is_null() {
        let next_event = (*iter).next_event;

        match (*iter).event_type {
            udi_event_type_e::UDI_EVENT_ERROR => {
                let error_event = (*iter).event_data as *const udi_event_error;
                libc::free((*error_event).errstr as *mut libc::c_void);
                libc::free(error_event as *mut libc::c_void);
            }
            udi_event_type_e::UDI_EVENT_PROCESS_EXEC => {
                let exec_event = (*iter).event_data as *const udi_event_process_exec;
                libc::free((*exec_event).path as *mut libc::c_void);
                free_null_term_array((*exec_event).argv);
                free_null_term_array((*exec_event).envp);
                libc::free(exec_event as *mut libc::c_void);
            }
            _ => {
                libc::free((*iter).event_data as *mut libc::c_void);
            }
        }

        libc::free(iter as *mut libc::c_void);

        iter = next_event;
    }
}

unsafe fn try_malloc(size: libc::size_t) -> Result<*mut libc::c_void, udi_error> {
    let ptr = libc::malloc(size);

    if !ptr.is_null() {
        Ok(ptr)
    } else {
        Err(udi_error {
            code: udi_error_e::UDI_ERROR_NOMEM,
            msg: std::ptr::null(),
        })
    }
}

macro_rules! cstr {
    ($s:expr) => {
        concat!($s, "\0") as *const str as *const [libc::c_char] as *const libc::c_char
    };
}

const UNKNOWN_STR: *const libc::c_char = cstr!("UNKNOWN");
const ERROR_STR: *const libc::c_char = cstr!("ERROR");
const SIGNAL: *const libc::c_char = cstr!("SIGNAL");
const BREAKPOINT: *const libc::c_char = cstr!("BREAKPOINT");
const THREAD_CREATE: *const libc::c_char = cstr!("THREAD_CREATE");
const THREAD_DEATH: *const libc::c_char = cstr!("THREAD_DEATH");
const PROCESS_EXIT: *const libc::c_char = cstr!("PROCESS_EXIT");
const PROCESS_FORK: *const libc::c_char = cstr!("PROCESS_FORK");
const PROCESS_EXEC: *const libc::c_char = cstr!("PROCESS_EXEC");
const SINGLE_STEP: *const libc::c_char = cstr!("SINGLE_STEP");
const PROCESS_CLEANUP: *const libc::c_char = cstr!("PROCESS_CLEANUP");

/// Gets the string representation of the specified event type.
///
/// # Arguments
///
/// * `input` - the event type to get the string representation for
///
/// # Returns
///
/// The string representation of the specified event type.
#[no_mangle]
pub unsafe extern "C" fn get_event_type_str(input: udi_event_type_e) -> *const libc::c_char {
    match input {
        udi_event_type_e::UDI_EVENT_ERROR => ERROR_STR,
        udi_event_type_e::UDI_EVENT_SIGNAL => SIGNAL,
        udi_event_type_e::UDI_EVENT_BREAKPOINT => BREAKPOINT,
        udi_event_type_e::UDI_EVENT_THREAD_CREATE => THREAD_CREATE,
        udi_event_type_e::UDI_EVENT_THREAD_DEATH => THREAD_DEATH,
        udi_event_type_e::UDI_EVENT_PROCESS_EXIT => PROCESS_EXIT,
        udi_event_type_e::UDI_EVENT_PROCESS_FORK => PROCESS_FORK,
        udi_event_type_e::UDI_EVENT_PROCESS_EXEC => PROCESS_EXEC,
        udi_event_type_e::UDI_EVENT_SINGLE_STEP => SINGLE_STEP,
        udi_event_type_e::UDI_EVENT_PROCESS_CLEANUP => PROCESS_CLEANUP,
        udi_event_type_e::UDI_EVENT_UNKNOWN => UNKNOWN_STR,
    }
}
