//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]
#![allow(non_camel_case_types)]

extern crate libc;
extern crate udi;

use std::sync::{Mutex,Arc};
use std::ffi::{CStr, CString};
use std::mem::{transmute, size_of};

use udi::{Process,
          ProcessConfig,
          Thread,
          Error,
          ErrorKind,
          UserData,
          EventData,
          Register};

pub struct udi_thread_struct {
    handle: Arc<Mutex<Thread>>,
    tid: u64,
    next: *const udi_thread_struct
}

pub struct udi_process_struct {
    handle: Arc<Mutex<Process>>,
    pid: u32,
    thr: *const udi_thread_struct
}

const UDI_ERROR_LIBRARY: libc::c_int = 0;
const UDI_ERROR_REQUEST: libc::c_int = 1;
const UDI_ERROR_NOMEM: libc::c_int = 2;
const UDI_ERROR_NONE: libc::c_int = 3;

#[repr(C)]
pub struct udi_error_struct {
    pub code: libc::c_int,
    pub msg: *const libc::c_schar,
}

trait UnsafeFrom<T> {
    unsafe fn from(_: T) -> Self;
}

impl UnsafeFrom<udi::Result<()>> for udi_error_struct {
    unsafe fn from(result: udi::Result<()>) -> udi_error_struct {
        match result {
            Ok(_) => udi_error_struct{ code: UDI_ERROR_NONE, msg: std::ptr::null() },
            Err(e) => match *e.kind() {
                ErrorKind::Request(ref msg) => {
                    udi_error_struct{ code: UDI_ERROR_REQUEST, msg: to_c_string(msg) }
                },
                _ => {
                    let msg = format!("{}", e);
                    udi_error_struct{ code: UDI_ERROR_LIBRARY, msg: to_c_string(&msg) }
                }
            }
        }
    }
}

impl UnsafeFrom<Error> for udi_error_struct {
    unsafe fn from(e: Error) -> udi_error_struct {
        match *e.kind() {
            ErrorKind::Request(ref msg) => {
                udi_error_struct{ code: UDI_ERROR_REQUEST, msg: to_c_string(&msg) }
            },
            _ => {
                let msg = format!("{}", e);
                udi_error_struct{ code: UDI_ERROR_LIBRARY, msg: to_c_string(&msg) }
            }
        }
    }
}

impl UnsafeFrom<udi_error_struct> for udi_error_struct {
    unsafe fn from(e: udi_error_struct) -> udi_error_struct {
        e
    }
}

impl<'a> UnsafeFrom<std::sync::PoisonError<std::sync::MutexGuard<'a, Process>>> for udi_error_struct {
    unsafe fn from(_: std::sync::PoisonError<std::sync::MutexGuard<'a, Process>>) -> udi_error_struct {
        udi_error_struct{ code: UDI_ERROR_LIBRARY, msg: to_c_string("lock failed") }
    }
}

impl<'a> UnsafeFrom<std::sync::PoisonError<std::sync::MutexGuard<'a, Thread>>> for udi_error_struct {
    unsafe fn from(_: std::sync::PoisonError<std::sync::MutexGuard<'a, Thread>>) -> udi_error_struct {
        udi_error_struct{ code: UDI_ERROR_LIBRARY, msg: to_c_string("lock failed") }
    }
}

unsafe fn to_c_string(msg: &str) -> *const libc::c_schar {
    let cstr = match CString::new(msg) {
        Ok(val) => val,
        Err(_) => return std::ptr::null(),
    };

    let len = cstr.to_bytes_with_nul().len();
    let output = libc::malloc(len);
    if output != std::ptr::null_mut() {
        libc::memcpy(output, cstr.as_ptr() as *const libc::c_void, len);
    }

    output as *const libc::c_schar
}

macro_rules! try_err {
    ($e:expr) => (match $e {
        Ok(val) => val,
        Err(err) => return UnsafeFrom::from(err)
    });
}

macro_rules! try_unsafe {
    ($e:expr) => (match $e {
        Ok(val) => val,
        Err(err) => return Err(UnsafeFrom::from(err))
    });
}

#[repr(C)]
pub struct udi_proc_config_struct {
    root_dir: *const libc::c_schar,
    rt_lib_path: *const libc::c_schar
}

#[no_mangle]
pub unsafe extern "C" fn create_process(executable: *const libc::c_schar,
                                        argv: *const *const libc::c_schar,
                                        envp: *const *const libc::c_schar,
                                        config: *const udi_proc_config_struct,
                                        process: *mut *const udi_process_struct)
    -> udi_error_struct
{
    if executable.is_null() {
        *process = std::ptr::null();
        return udi_error_struct{
            code: UDI_ERROR_REQUEST,
            msg: to_c_string("Executable cannot be null")
        };
    }

    if argv.is_null() {
        *process = std::ptr::null();
        return udi_error_struct{
            code: UDI_ERROR_REQUEST,
            msg: to_c_string("Argument array cannot be null")
        };
    }

    if config.is_null() {
        *process = std::ptr::null();
        return udi_error_struct{
            code: UDI_ERROR_REQUEST,
            msg: to_c_string("Process config cannot be null")
        };
    }

    let exec_str = match CStr::from_ptr(executable).to_str() {
        Ok(val) => val,
        Err(_) => {
            *process = std::ptr::null();
            return udi_error_struct{
                code: UDI_ERROR_REQUEST,
                msg: to_c_string("Executable is not a valid UTF-8 string")
            };
        }
    };

    let argv_vec = match to_vec(argv) {
        Ok(val) => val,
        Err(msg) => {
            *process = std::ptr::null();
            return udi_error_struct{
                code: UDI_ERROR_REQUEST,
                msg: msg
            };
        }
    };

    let envp_vec;
    if envp != std::ptr::null() {
        envp_vec = match to_vec(envp) {
            Ok(val) => val,
            Err(msg) => {
                *process = std::ptr::null();
                return udi_error_struct{
                    code: UDI_ERROR_REQUEST,
                    msg
                };
            }
        };
    }else{
        envp_vec = vec![];
    }

    let root_dir_str;
    if (*config).root_dir != std::ptr::null() {
        root_dir_str = match CStr::from_ptr((*config).root_dir).to_str() {
            Ok(val) => Some(val.to_owned()),
            Err(_) => {
                *process = std::ptr::null();
                return udi_error_struct{
                    code: UDI_ERROR_REQUEST,
                    msg: to_c_string("Process config root dir is not a valid UTF-8 string")
                };
            }
        };
    }else{
        root_dir_str = None;
    }

    let rt_lib_path_str;
    if (*config).rt_lib_path != std::ptr::null() {
        rt_lib_path_str = match CStr::from_ptr((*config).rt_lib_path).to_str() {
            Ok(val) => Some(val.to_owned()),
            Err(_) => {
                *process = std::ptr::null();
                return udi_error_struct{
                    code: UDI_ERROR_REQUEST,
                    msg: to_c_string("Process config rt lib path is not a valid UTF-8 string")
                };
            }
        };
    }else{
        rt_lib_path_str = None;
    }

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

                thread_ptr = Box::new(udi_thread_struct{
                    handle: udi_process.get_initial_thread(),
                    tid,
                    next: std::ptr::null()
                });
            }

            let process_ptr = Box::new(udi_process_struct{
                handle: p,
                pid,
                thr: Box::into_raw(thread_ptr)
            });
            *process = Box::into_raw(process_ptr);

            udi_error_struct{
                code: UDI_ERROR_NONE,
                msg: std::ptr::null()
            }
        },
        Err(e) => {
            *process = std::ptr::null();

            UnsafeFrom::from(e)
        }
    }
}

unsafe fn to_vec(arr: *const *const libc::c_schar)
    -> std::result::Result<Vec<String>, *const libc::c_schar> {

    let mut x = 0;

    let mut output: Vec<String> = vec![];
    while *arr.offset(x) != std::ptr::null() {
        let elem = *arr.offset(x);

        let elem_str = match CStr::from_ptr(elem).to_str() {
            Ok(val) => val.to_owned(),
            Err(_) => return Err(to_c_string("Invalid string in specified array")),
        };

        output.push(elem_str);

        x = x + 1;
    }

    Ok(output)
}

#[no_mangle]
pub unsafe extern "C"
fn free_process(process: *const udi_process_struct) {

    let mut thr_ptr = (*process).thr;
    while thr_ptr != std::ptr::null() {
        let next_ptr = (*thr_ptr).next;

        drop(Box::from_raw(thr_ptr as *mut udi_thread_struct));

        thr_ptr = next_ptr;
    }

    let process_ptr = Box::from_raw(process as *mut udi_process_struct);
    drop(process_ptr);
}

#[no_mangle]
pub unsafe extern "C"
fn continue_process(process_wrapper: *const udi_process_struct) -> udi_error_struct {
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.continue_process())
}

#[no_mangle]
pub unsafe extern "C"
fn refresh_state(process_wrapper: *const udi_process_struct) -> udi_error_struct {
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.refresh_state())
}

#[derive(Debug)]
struct CUserData {
    pub ptr: *const libc::c_void
}

impl UserData for CUserData {}

#[no_mangle]
pub unsafe extern "C"
fn set_user_data(process_wrapper: *const udi_process_struct, data: *const libc::c_void)
    -> udi_error_struct
{
    let data_ptr = Box::new(CUserData{ ptr: data });

    let mut process = try_err!((*process_wrapper).handle.lock());

    process.set_user_data(data_ptr);

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_user_data(process_wrapper: *const udi_process_struct, data: *mut *const libc::c_void)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    let user_data = process.get_user_data()
        .and_then(|d| d.downcast_ref::<CUserData>())
        .map(|c| c.ptr)
        .unwrap_or(std::ptr::null());

    *data = user_data;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_proc_pid(process_wrapper: *const udi_process_struct, output: *mut libc::uint32_t)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = process.get_pid();

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_proc_architecture(process_wrapper: *const udi_process_struct, output: *mut libc::uint32_t)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = process.get_architecture() as u32;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_multithread_capable(process_wrapper: *const udi_process_struct, output: *mut libc::int32_t)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = process.is_multithread_capable() as i32;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_initial_thread(process_wrapper: *const udi_process_struct,
                      output: *mut *const udi_thread_struct)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = (*process_wrapper).thr;

    drop(process);

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn is_running(process_wrapper: *const udi_process_struct, output: *mut libc::int32_t)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = process.is_running() as i32;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn is_terminated(process_wrapper: *const udi_process_struct, output: *mut libc::int32_t)
    -> udi_error_struct
{
    let process = try_err!((*process_wrapper).handle.lock());

    *output = process.is_terminated() as i32;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn set_thread_user_data(thr_wrapper: *const udi_thread_struct, data: *const libc::c_void)
    -> udi_error_struct
{
    let user_data = Box::new(CUserData{ ptr: data });

    let mut thr = try_err!((*thr_wrapper).handle.lock());

    thr.set_user_data(user_data);

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_thread_user_data(thr_wrapper: *const udi_thread_struct, data: *mut *const libc::c_void)
    -> udi_error_struct
{
    let mut thr = try_err!((*thr_wrapper).handle.lock());

    let user_data = thr.get_user_data()
        .and_then(|d| d.downcast_ref::<CUserData>())
        .map(|c| c.ptr)
        .unwrap_or(std::ptr::null());

    *data = user_data;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_tid(thr_wrapper: *const udi_thread_struct, output: *mut libc::uint64_t)
    -> udi_error_struct
{
    let thr = try_err!((*thr_wrapper).handle.lock());

    *output = thr.get_tid();

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_state(thr_wrapper: *const udi_thread_struct, output: *mut libc::uint32_t)
    -> udi_error_struct
{
    let thr = try_err!((*thr_wrapper).handle.lock());

    *output = thr.get_state() as u32;

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_next_thread(proc_wrapper: *const udi_process_struct,
                   thr_wrapper: *const udi_thread_struct,
                   output: *mut *const udi_thread_struct)
    -> udi_error_struct
{
    let process = try_err!((*proc_wrapper).handle.lock());

    *output = (*thr_wrapper).next;

    drop(process);

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn resume_thread(thr_wrapper: *const udi_thread_struct)
    -> udi_error_struct
{
    let mut thr = try_err!((*thr_wrapper).handle.lock());

    UnsafeFrom::from(thr.resume())
}

#[no_mangle]
pub unsafe extern "C"
fn suspend_thread(thr_wrapper: *const udi_thread_struct)
    -> udi_error_struct
{
    let mut thr = try_err!((*thr_wrapper).handle.lock());

    UnsafeFrom::from(thr.suspend())
}

#[no_mangle]
pub unsafe extern "C"
fn set_single_step(thr_wrapper: *const udi_thread_struct, enable: libc::c_int)
    -> udi_error_struct
{
    let mut thr = try_err!((*thr_wrapper).handle.lock());

    UnsafeFrom::from(thr.set_single_step(enable != 0))
}

#[no_mangle]
pub unsafe extern "C"
fn get_single_step(thr_wrapper: *const udi_thread_struct, output: *mut libc::int32_t)
    -> udi_error_struct
{
    let thr = try_err!((*thr_wrapper).handle.lock());

    *output = thr.get_single_step() as i32;

    UnsafeFrom::from(Ok(()))
}


#[no_mangle]
pub unsafe extern "C"
fn create_breakpoint(process_wrapper: *const udi_process_struct, addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.create_breakpoint(addr))
}

#[no_mangle]
pub unsafe extern "C"
fn install_breakpoint(process_wrapper: *const udi_process_struct, addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.install_breakpoint(addr))
}

#[no_mangle]
pub unsafe extern "C"
fn remove_breakpoint(process_wrapper: *const udi_process_struct, addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.remove_breakpoint(addr))
}

#[no_mangle]
pub unsafe extern "C"
fn delete_breakpoint(process_wrapper: *const udi_process_struct, addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    UnsafeFrom::from(process.delete_breakpoint(addr))
}

#[no_mangle]
pub unsafe extern "C"
fn read_mem(process_wrapper: *const udi_process_struct,
            dst: *mut libc::uint8_t,
            size: libc::uint32_t,
            addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    let data = try_err!(process.read_mem(size, addr));
    let src = &data[0] as *const libc::uint8_t;
    libc::memcpy(dst as *mut libc::c_void, src as *const libc::c_void, size as usize);
    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn write_mem(process_wrapper: *const udi_process_struct,
             src: *const libc::uint8_t,
             size: libc::uint32_t,
             addr: libc::uint64_t)
    -> udi_error_struct
{
    let mut process = try_err!((*process_wrapper).handle.lock());

    let src = std::slice::from_raw_parts(src, size as usize);
    UnsafeFrom::from(process.write_mem(src, addr))
}

#[no_mangle]
pub unsafe extern "C"
fn read_register(thr_wrapper: *const udi_thread_struct,
                 reg: libc::uint32_t,
                 value: *mut libc::uint64_t)
    -> udi_error_struct
{
    let mut thread = try_err!((*thr_wrapper).handle.lock());

    let register = transmute::<libc::uint32_t, Register>(reg);

    *value = try_err!(thread.read_register(register));

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn write_register(thr_wrapper: *const udi_thread_struct,
                  reg: libc::uint32_t,
                  value: libc::uint64_t)
    -> udi_error_struct
{
    let mut thread = try_err!((*thr_wrapper).handle.lock());

    let register = transmute::<libc::uint32_t, Register>(reg);

    UnsafeFrom::from(thread.write_register(register, value))
}

#[no_mangle]
pub unsafe extern "C"
fn get_pc(thr_wrapper: *const udi_thread_struct,
          pc: *mut libc::uint64_t)
    -> udi_error_struct
{
    let mut thread = try_err!((*thr_wrapper).handle.lock());

    *pc = try_err!(thread.get_pc());

    UnsafeFrom::from(Ok(()))
}

#[no_mangle]
pub unsafe extern "C"
fn get_next_instruction(thr_wrapper: *const udi_thread_struct,
                        instr: *mut libc::uint64_t)
    -> udi_error_struct
{
    let mut thread = try_err!((*thr_wrapper).handle.lock());

    *instr = try_err!(thread.get_next_instruction());

    UnsafeFrom::from(Ok(()))
}

#[repr(C)]
pub struct udi_event_struct {
    event_type: libc::uint32_t,
    process: *const udi_process_struct,
    thr: *const udi_thread_struct,
    event_data: *const libc::c_void,
    next_event: *const udi_event_struct
}

#[repr(C)]
pub struct udi_event_error_struct {
    errstr: *const libc::c_schar
}

#[repr(C)]
pub struct udi_event_process_exit_struct {
    exit_code: libc::int32_t
}

#[repr(C)]
pub struct udi_event_breakpoint_struct {
    breakpoint_addr: libc::uint64_t
}

#[repr(C)]
pub struct udi_event_thread_create_struct {
    new_thr: *const udi_thread_struct
}

#[repr(C)]
pub struct udi_event_signal_struct {
    addr: libc::uint64_t,
    sig: libc::uint32_t
}

#[repr(C)]
pub struct udi_event_process_fork_struct {
    pid: libc::uint32_t
}

#[repr(C)]
pub struct udi_event_process_exec_struct {
    path: *const libc::c_schar,
    argv: *const *const libc::c_schar,
    envp: *const *const libc::c_schar
}

#[no_mangle]
pub unsafe extern "C"
fn wait_for_events(procs: *const *const udi_process_struct,
                   num_procs: libc::c_int,
                   output: *mut *const udi_event_struct)
    -> udi_error_struct
{
    let procs_slice: &[* const udi_process_struct] = std::slice::from_raw_parts(procs, num_procs as usize);

    let procs_vec = procs_slice.iter()
        .map(|p| (**p).handle.clone())
        .collect::<Vec< Arc<Mutex<Process>>>>();

    let events = try_err!(udi::wait_for_events(&procs_vec));

    let mut output_events;
    if events.len() > 0 {

        output_events = std::ptr::null();
        let mut last_event = std::ptr::null() as *const udi_event_struct;
        for event in events {
            let proc_handle = match find_proc_handle(&procs_slice, &event.process) {
                Ok(p) => p,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

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

            let current_event = match try_malloc(size_of::<udi_event_struct>()) {
                Ok(p) => p as *mut udi_event_struct,
                Err(e) => {
                    free_event_list(output_events);
                    return e;
                }
            };

            *current_event = event_struct;
            (*current_event).next_event = last_event;
            last_event = current_event;

            if output_events == std::ptr::null() {
                output_events = current_event;
            }
        }
    } else {
        output_events = std::ptr::null();
    }

    *output = output_events;

    UnsafeFrom::from(Ok(()))
}

unsafe fn find_proc_handle(procs_slice: &[*const udi_process_struct],
                           process: &Arc<Mutex<Process>>)
    -> Result<* const udi_process_struct, udi_error_struct> {

    let pid = try_unsafe!(process.lock()).get_pid();

    let opt = procs_slice.iter().find(|&&p| (*p).pid == pid).map(|p| *p);

    opt.ok_or_else(|| udi_error_struct{
        code: UDI_ERROR_LIBRARY,
        msg: to_c_string("Could not locate event process handle")
    })
}

unsafe fn find_thr_handle(proc_handle: *const udi_process_struct,
                          thread: &Arc<Mutex<Thread>>)
    -> Result<* const udi_thread_struct, udi_error_struct> {

    let tid = try_unsafe!(thread.lock()).get_tid();

    find_thr_handle_for_tid(proc_handle, tid)
}
unsafe fn find_thr_handle_for_tid(proc_handle: *const udi_process_struct, tid: u64)
    -> Result<* const udi_thread_struct, udi_error_struct> {

    let mut thr_iter = (*proc_handle).thr;

    let mut opt = None;
    while thr_iter != std::ptr::null() {
        if (*thr_iter).tid == tid {
            opt = Some(thr_iter);
            break;
        }

        thr_iter = (*thr_iter).next;
    }

    opt.ok_or_else(|| udi_error_struct{
        code: UDI_ERROR_LIBRARY,
        msg: to_c_string("Could not locate event thread handle")
    })
}

unsafe
fn to_event_struct(proc_handle: *const udi_process_struct,
                   thr_handle: *const udi_thread_struct,
                   event_data: &EventData)
    -> Result<udi_event_struct, udi_error_struct>
{
    let (event_type, raw_event_data) = match *event_data {
        EventData::Error{ ref msg } => {
            let error_struct = try_malloc(size_of::<udi_event_error_struct>())?
                as *mut udi_event_error_struct;

            (*error_struct).errstr = to_c_string(&msg);

            (UDI_EVENT_ERROR, error_struct as *const libc::c_void)
        },
        EventData::Signal{ addr, sig } => {
            let signal_struct = try_malloc(size_of::<udi_event_signal_struct>())?
                as *mut udi_event_signal_struct;

            (*signal_struct).addr = addr;
            (*signal_struct).sig = sig;

            (UDI_EVENT_SIGNAL, std::ptr::null())
        },
        EventData::Breakpoint{ addr } => {
            let brkpt_struct = try_malloc(size_of::<udi_event_breakpoint_struct>())?
                as *mut udi_event_breakpoint_struct;

            (*brkpt_struct).breakpoint_addr = addr;

            (UDI_EVENT_BREAKPOINT, brkpt_struct as *const libc::c_void)
        },
        EventData::ThreadCreate{ tid } => {
            let thr_struct = try_malloc(size_of::<udi_event_thread_create_struct>())?
                as *mut udi_event_thread_create_struct;

            (*thr_struct).new_thr = find_thr_handle_for_tid(proc_handle, tid)?;

            (UDI_EVENT_THREAD_CREATE, thr_struct as *const libc::c_void)
        },
        EventData::ThreadDeath => (UDI_EVENT_THREAD_DEATH, std::ptr::null()),
        EventData::ProcessExit{ code } => {
            let exit_struct = try_malloc(size_of::<udi_event_process_exit_struct>())?
                as *mut udi_event_process_exit_struct;

            (*exit_struct).exit_code = code;

            (UDI_EVENT_PROCESS_EXIT, exit_struct as *const libc::c_void)
        },
        EventData::ProcessFork{ pid } => {
            let fork_struct = try_malloc(size_of::<udi_event_process_fork_struct>())?
                as *mut udi_event_process_fork_struct;

            (*fork_struct).pid = pid;

            (UDI_EVENT_PROCESS_FORK, fork_struct as *const libc::c_void)
        },
        EventData::ProcessExec{ ref path, ref argv, ref envp } => {
            let exec_struct = try_malloc(size_of::<udi_event_process_exec_struct>())?
                as *mut udi_event_process_exec_struct;

            (*exec_struct).argv = match to_null_term_array(&argv) {
                Ok(argv) => argv,
                Err(e) => {
                    libc::free(exec_struct as *mut libc::c_void);
                    return Err(e);
                }
            };

            (*exec_struct).envp = match to_null_term_array(&envp) {
                Ok(envp) => envp,
                Err(e) => {
                    free_null_term_array((*exec_struct).argv);
                    libc::free(exec_struct as *mut libc::c_void);
                    return Err(e);
                }
            };

            (*exec_struct).path = to_c_string(&path);

            if (*exec_struct).path == std::ptr::null() {
                free_null_term_array((*exec_struct).argv);
                free_null_term_array((*exec_struct).envp);
                libc::free(exec_struct as *mut libc::c_void);
                return Err(udi_error_struct{
                    code: UDI_ERROR_NOMEM,
                    msg: std::ptr::null()
                });
            }

            (UDI_EVENT_PROCESS_EXEC, exec_struct as *const libc::c_void)
        },
        EventData::SingleStep => (UDI_EVENT_SINGLE_STEP, std::ptr::null()),
        EventData::ProcessCleanup => (UDI_EVENT_PROCESS_CLEANUP, std::ptr::null())
    };

    Ok(udi_event_struct{
        process: proc_handle,
        thr: thr_handle,
        event_type,
        event_data: raw_event_data,
        next_event: std::ptr::null()
    })
}

unsafe
fn to_null_term_array(input: &Vec<String>)
    -> Result<*const *const libc::c_schar, udi_error_struct> {

    let len = input.len();
    let output = try_malloc(size_of::<*const libc::c_schar>() * (len + 1))?
        as *mut *const libc::c_schar;
    *(output.offset(len as isize)) = std::ptr::null();

    for (i, elem) in input.iter().enumerate() {
        let elem_c_str = to_c_string(&elem);
        if elem_c_str == std::ptr::null() {
            free_null_term_array(output);
            return Err(udi_error_struct{
                code: UDI_ERROR_NOMEM,
                msg: std::ptr::null()
            });
        }
        *(output.offset(i as isize)) = elem_c_str;
    }

    Ok(output)
}

unsafe
fn free_null_term_array(input: *const *const libc::c_schar) {

    let mut index = 0;
    loop {
        let current = *(input.offset(index));
        if current != std::ptr::null() {
            libc::free(current as *mut libc::c_void);
            index += 1;
        } else {
            break;
        }
    }

    libc::free(input as *mut libc::c_void);
}

#[no_mangle]
pub unsafe extern "C"
fn free_event_list(event_list: *const udi_event_struct)
{
    let mut iter = event_list;

    while iter != std::ptr::null() {
        let next_event = (*iter).next_event;

        match (*iter).event_type {
            UDI_EVENT_ERROR => {
                let error_event = (*iter).event_data as *const udi_event_error_struct;
                libc::free((*error_event).errstr as *mut libc::c_void);
                libc::free(error_event as *mut libc::c_void);
            },
            UDI_EVENT_PROCESS_EXEC => {
                let exec_event = (*iter).event_data as *const udi_event_process_exec_struct;
                libc::free((*exec_event).path as *mut libc::c_void);
                free_null_term_array((*exec_event).argv);
                free_null_term_array((*exec_event).envp);
                libc::free(exec_event as *mut libc::c_void);
            },
            _ => {
                libc::free((*iter).event_data as *mut libc::c_void);
            }
        }

        libc::free(iter as *mut libc::c_void);

        iter = next_event;
    }
}

unsafe
fn try_malloc(size: libc::size_t) -> Result<*mut libc::c_void, udi_error_struct> {
    let ptr = libc::malloc(size);

    if ptr != std::ptr::null_mut() {
        Ok(ptr)
    } else {
        Err(udi_error_struct{ code: UDI_ERROR_NOMEM, msg: std::ptr::null() })
    }
}

macro_rules! cstr {
  ($s:expr) => (
    concat!($s, "\0") as *const str as *const [libc::c_char] as *const libc::c_char
  );
}

const UDI_EVENT_UNKNOWN: libc::uint32_t = 0;
const UNKNOWN_STR: *const libc::c_char = cstr!("UNKNOWN");

const UDI_EVENT_ERROR: libc::uint32_t = 1;
const ERROR_STR: *const libc::c_char = cstr!("ERROR");

const SIGNAL: *const libc::c_char = cstr!("SIGNAL");
const UDI_EVENT_SIGNAL: libc::uint32_t = 2;

const BREAKPOINT: *const libc::c_char = cstr!("BREAKPOINT");
const UDI_EVENT_BREAKPOINT: libc::uint32_t = 3;

const THREAD_CREATE: *const libc::c_char = cstr!("THREAD_CREATE");
const UDI_EVENT_THREAD_CREATE: libc::uint32_t = 4;

const THREAD_DEATH: *const libc::c_char = cstr!("THREAD_DEATH");
const UDI_EVENT_THREAD_DEATH: libc::uint32_t = 5;

const PROCESS_EXIT: *const libc::c_char = cstr!("PROCESS_EXIT");
const UDI_EVENT_PROCESS_EXIT: libc::uint32_t = 6;

const PROCESS_FORK: *const libc::c_char = cstr!("PROCESS_FORK");
const UDI_EVENT_PROCESS_FORK: libc::uint32_t = 7;

const PROCESS_EXEC: *const libc::c_char = cstr!("PROCESS_EXEC");
const UDI_EVENT_PROCESS_EXEC: libc::uint32_t = 8;

const SINGLE_STEP: *const libc::c_char = cstr!("SINGLE_STEP");
const UDI_EVENT_SINGLE_STEP: libc::uint32_t = 9;

const PROCESS_CLEANUP: *const libc::c_char = cstr!("PROCESS_CLEANUP");
const UDI_EVENT_PROCESS_CLEANUP: libc::uint32_t = 10;

#[no_mangle]
pub unsafe extern "C"
fn get_event_type_str(input: libc::uint32_t) -> *const libc::c_schar
{
    match input {
        UDI_EVENT_ERROR => ERROR_STR,
        UDI_EVENT_SIGNAL => SIGNAL,
        UDI_EVENT_BREAKPOINT => BREAKPOINT,
        UDI_EVENT_THREAD_CREATE => THREAD_CREATE,
        UDI_EVENT_THREAD_DEATH => THREAD_DEATH,
        UDI_EVENT_PROCESS_EXIT => PROCESS_EXIT,
        UDI_EVENT_PROCESS_FORK => PROCESS_FORK,
        UDI_EVENT_PROCESS_EXEC => PROCESS_EXEC,
        UDI_EVENT_SINGLE_STEP => SINGLE_STEP,
        UDI_EVENT_PROCESS_CLEANUP => PROCESS_CLEANUP,
        UDI_EVENT_UNKNOWN => UNKNOWN_STR,
        _ => UNKNOWN_STR
    }
}
