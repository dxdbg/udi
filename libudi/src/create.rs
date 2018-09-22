//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

extern crate users;

use std::string::String;
use std::sync::{Mutex, Arc};
use std::path::{PathBuf, Path};
use std::io::{self,Write};
use std::fs;

use super::errors::*;
use super::Process;
use super::ProcessFileContext;
use super::Thread;
use super::ThreadFileContext;
use super::ThreadState;
use super::protocol;
use super::protocol::request;
use super::protocol::response;

const DEFAULT_UDI_ROOT_DIR: &'static str = "/tmp/udi";
const DEFAULT_UDI_RT_LIB_NAME: &'static str = "libudirt.so";
const UDI_ROOT_DIR_ENV: &'static str = "UDI_ROOT_DIR";
const REQUEST_FILE_NAME: &'static str = "request";
const RESPONSE_FILE_NAME: &'static str = "response";
const EVENTS_FILE_NAME: &'static str = "events";

#[derive(Debug)]
pub struct ProcessConfig {
    root_dir: Option<String>,
    rt_lib_path: Option<String>
}

impl ProcessConfig {
    pub fn new(root_dir: Option<String>,
               rt_lib_path: Option<String>) -> ProcessConfig {

        ProcessConfig{
            root_dir,
            rt_lib_path
        }
    }
}

/// Creates a new UDI-controlled process
pub fn create_process(executable: &str,
                      argv: &Vec<String>,
                      envp: &Vec<String>,
                      config: &ProcessConfig) -> Result<Arc<Mutex<Process>>> {

    let base_root_dir = config.root_dir.clone().unwrap_or(DEFAULT_UDI_ROOT_DIR.to_owned());
    let rt_lib_path = config.rt_lib_path.clone().unwrap_or(DEFAULT_UDI_RT_LIB_NAME.to_owned());

    let root_dir = create_root_udi_filesystem(&base_root_dir)?;

    let mut child = launch_process(executable,
                                   argv,
                                   envp,
                                   &root_dir,
                                   &rt_lib_path)?;
    let pid = child.id();

    let mut root_dir_buf = PathBuf::from(&root_dir);
    root_dir_buf.push(pid.to_string());

    let process = initialize_process(&mut child, (*root_dir_buf.to_string_lossy()).to_owned())?;

    Ok(Arc::new(Mutex::new(process)))
}

fn initialize_process(child: &mut ::std::process::Child, root_dir: String)
    -> Result<Process> {

    let pid = child.id();

    let mut request_path_buf = PathBuf::from(&root_dir);
    request_path_buf.push(REQUEST_FILE_NAME);
    let request_path = request_path_buf.as_path();

    let mut response_path_buf = PathBuf::from(&root_dir);
    response_path_buf.push(RESPONSE_FILE_NAME);
    let response_path = response_path_buf.as_path();

    let mut event_path_buf = PathBuf::from(&root_dir);
    event_path_buf.push(EVENTS_FILE_NAME);
    let event_path = event_path_buf.as_path();

    // poll for change in root UDI filesystem
    // TODO use notify crate for this
    let mut event_file_exists = false;
    while !event_file_exists {
        match child.try_wait()? {
            Some(status) => {
                let msg = format!("Process failed to initialize, exited with status: {}", status);
                return Err(ErrorKind::Library(msg).into());
            },
            None => {
            }
        };

        match fs::metadata(event_path) {
            Ok(_) => {
                event_file_exists = true;
            },
            Err(e) => match e.kind() {
                io::ErrorKind::NotFound => {},
                _ => return Err(::std::convert::From::from(e))
            }
        };
    };

    // order matters here because POSIX FIFOs block in open calls

    let mut request_file = fs::OpenOptions::new().read(false)
                                                 .create(false)
                                                 .write(true)
                                                 .open(request_path)?;

    request_file.write_all(&protocol::request::serialize(&request::Init::new())?)?;

    let mut response_file = fs::File::open(response_path)?;
    let events_file = fs::File::open(event_path)?;

    let init: response::Init = protocol::response::read(&mut response_file)?;

    // Check compatibility with protocol version
    let version = determine_protocol(&init)?;

    let mut process = Process {
        pid: pid,
        file_context: Some(ProcessFileContext{ request_file, response_file, events_file }),
        architecture: init.arch,
        protocol_version: version,
        multithread_capable: init.mt,
        running: false,
        terminating: false,
        user_data: None,
        threads: vec![],
        root_dir: root_dir,
    };

    initialize_thread(&mut process, init.tid)?;

    Ok(process)
}

fn determine_protocol(init: &response::Init) -> Result<u32> {
    if init.v != protocol::UDI_PROTOCOL_VERSION_1 {
        let msg = format!("Debuggee expects unsupported protocol version: {}", init.v);
        return Err(ErrorKind::Library(msg).into());
    }

    Ok(init.v)
}

/// Performs the init handshake for the new thread and adds it to the specified process
pub fn initialize_thread(process: &mut Process, tid: u64) -> Result<()> {
    let mut request_path_buf = PathBuf::from(&process.root_dir);
    request_path_buf.push(format!("{:016x}", tid));
    request_path_buf.push(REQUEST_FILE_NAME);
    let request_path = request_path_buf.as_path();

    let mut response_path_buf = PathBuf::from(&process.root_dir);
    response_path_buf.push(format!("{:016x}", tid));
    response_path_buf.push(RESPONSE_FILE_NAME);
    let response_path = response_path_buf.as_path();

    let mut request_file = fs::OpenOptions::new().read(false)
                                                 .create(false)
                                                 .write(true)
                                                 .open(request_path)?;

    request_file.write_all(&protocol::request::serialize(&request::Init::new())?)?;

    let mut response_file = fs::File::open(response_path)?;

    protocol::response::read::<response::Init, _>(&mut response_file)?;

    let thr = Thread {
        initial: process.threads.len() == 0,
        tid: tid,
        file_context: Some(ThreadFileContext{ request_file, response_file }),
        single_step: false,
        state: ThreadState::Running,
        architecture: process.architecture,
        user_data: None
    };

    process.threads.push(Arc::new(Mutex::new(thr)));

    Ok(())
}

/// Launch the UDI-controlled process
fn launch_process(executable: &str,
                  argv: &Vec<String>,
                  envp: &Vec<String>,
                  root_dir: &str,
                  rt_lib_path: &str) -> Result<::std::process::Child> {

    let mut command = ::std::process::Command::new(executable);

    let env = create_environment(envp,
                                 root_dir,
                                 rt_lib_path);

    for entry in env {
        command.env(entry.0, entry.1);
    }

    for entry in argv {
        command.arg(entry);
    }

    let child = command.spawn()?;

    Ok(child)
}



/// Creates the root UDI filesystem for the user
fn create_root_udi_filesystem(root_dir: &String) -> Result<String> {
    mkdir_ignore_exists(Path::new(root_dir))?;

    let user = users::get_current_username()
        .ok_or(Error::from_kind(ErrorKind::Library("Failed to retrieve username".to_owned())))?;

    let mut user_dir_path = PathBuf::from(root_dir);
    user_dir_path.push(user);

    mkdir_ignore_exists(user_dir_path.as_path())?;

    if let Some(user_dir_path_str) = user_dir_path.to_str() {
        Ok(user_dir_path_str.to_owned())
    } else {
        Err(ErrorKind::Library("Invalid root UDI filesystem path".to_owned()).into())
    }
}

/// Create the specified directory, ignoring the error if it already exists
fn mkdir_ignore_exists(dir: &Path) -> Result<()> {
    match fs::create_dir(dir) {
        Ok(_) => Ok(()),
        Err(e) => match e.kind() {
            io::ErrorKind::AlreadyExists => Ok(()),
            _ => Err(::std::convert::From::from(e))
        }
    }
}

/// Adds the UDI RT library into the appropriate dynamic linker environment variable.
/// The variable is created at the end of the array if it does not already exist. Adds the
/// UDI_ROOT_DIR_ENV environment variable to the end of the array after the dynamic
/// linker environment variable. This environment variable is replaced if it already exists.
fn create_environment(envp: &Vec<String>,
                      root_dir: &str,
                      rt_lib_path: &str) -> Vec<(String, String)> {
    let mut output;
    if envp.len() == 0 {
        output = vec![];
    } else {
        output = envp.iter().map(|s| {
            let mut field_iter = s.split("=");
            field_iter.next()
                .map(|k| (k.to_owned(),
                          field_iter.fold(String::new(), |mut l, r| {
                              l.push_str(r);
                              l
                          })))
                .unwrap_or((s.clone(), String::new()))
        }).collect::<Vec<_>>();
    }

    let var_name = sys::get_dyn_linker_var_name();

    // Update dynamic linker variable to include the runtime library
    let mut updated_var = false;
    for item in &mut output {
        let k = &item.0;
        let v = &mut item.1;

        if k == var_name {
            v.push_str(":");
            v.push_str(rt_lib_path);
            updated_var = true;
        }
    }

    if !updated_var {
        output.push((var_name.to_owned(), rt_lib_path.to_owned()));
    }

    output.push((UDI_ROOT_DIR_ENV.to_owned(), root_dir.to_owned()));

    sys::modify_env(&mut output);

    output
}

#[cfg(target_os = "linux")]
mod sys {
    pub fn get_dyn_linker_var_name() -> Option<&'static str> {
        Some("LD_PRELOAD")
    }

    pub fn modify_env(_env: &mut Vec<(String, String)>) {
    }

    pub fn spawn(mut cmd: ::std::process::Command) -> Result<::std::process::Child> {
        cmd.spawn()
    }
}

#[cfg(target_os = "macos")]
mod sys {
    pub fn get_dyn_linker_var_name() -> Option<&'static str> {
        Some("DYLD_INSERT_LIBRARIES")
    }

    pub fn modify_env(env: &mut Vec<(String, String)>) {
        env.push(("DYLD_FORCE_FLAT_NAMESPACE".to_owned(), "1".to_owned()));
    }

    pub fn spawn(mut cmd: ::std::process::Command) -> Result<::std::process::Child> {
        cmd.spawn()
    }
}

#[cfg(windows)]
mod sys {
    extern crate winapi;

    use ::std::ptr::null_mut;
    use ::std::ffi::CString;

    use ::std::os::windows::process::CommandExt;
    use ::std::os::windows::io::AsRawHandle;

    use create::sys::winapi::um::memoryapi::{
        VirtualAllocEx,
        WriteProcessMemory
    };
    use create::sys::winapi::um::libloaderapi::{
        GetProcAddress,
        GetModuleHandleA
    };
    use create::sys::winapi::um::errhandlingapi::{
        GetLastError
    };
    use create::sys::winapi::um::winnt::{
        PAGE_READWRITE,
        MEM_COMMIT
    };
    use create::sys::winapi::um::winbase::{
        INFINITE,
        WAIT_FAILED
    };
    use create::sys::winapi::um::processthreadsapi::{
        CreateRemoteThread,
        GetExitCodeThread
    };
    use create::sys::winapi::um::synchapi::{
        WaitForSingleObject
    };
    use create::sys::winapi::um::handleapi::{
        INVALID_HANDLE_VALUE,
        CloseHandle
    };

    use create::sys::winapi::ctypes::c_void;

    pub fn get_dyn_linker_var_name() -> Option<&'static str> {
        None
    }

    pub fn modify_env(env: &mut Vec<(String, String)>) {

    }

    pub fn spawn(mut cmd: ::std::process::Command,
                 rt_lib_path: &str) -> Result<::std::process::Child>
    {
        let child = cmd.creation_flags(4).spawn();

        load_dll(child, rt_lib_path)?;

        Ok(child)
    }

    fn load_dll(child: &::std::process::Child, rt_lib_path: &str) -> Result<()> {
        unsafe {
            let handle = child.as_raw_handle() as *mut c_void;

            let dll_path_vec: Vec<u8> = dll_path.into();
            let dll_path_len = dll_path_vec.len();

            let dll_path_str = CString::new(dll_path_vec).unwrap();

            let addr = VirtualAllocEx(handle,
                                      null_mut(),
                                      dll_path_len + 1,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);

            if addr == null_mut() {
                return to_library_error("Failed to allocate memory");

            }

            let write_result = WriteProcessMemory(handle,
                                                  addr,
                                                  dll_path_str.as_ptr() as *const c_void,
                                                  dll_path_len,
                                                  null_mut());

            if write_result == 0 {
                return to_library_error("Failed to write process memory");
            }

            let kernel32_str = CString::new("kernel32.dll").unwrap();
            let kernel32_handle = GetModuleHandleA(kernel32_str.as_ptr());
            if kernel32_handle == INVALID_HANDLE_VALUE {
                return to_library_error("Failed to get handle to kernel32.dll");
            }

            let load_library_str = CString::new("LoadLibraryA").unwrap();
            let load_library_address_result = GetProcAddress(kernel32_handle, load_library_str.as_ptr());
            let load_library_address: unsafe extern "system" fn(*mut c_void) -> u32 =
                ::std::mem::transmute(load_library_address_result as *const c_void);

            let thread = CreateRemoteThread(handle,
                                            null_mut(),
                                            0,
                                            Some(load_library_address),
                                            addr,
                                            0,
                                            null_mut());
            if thread == INVALID_HANDLE_VALUE {
                return to_library_error("Failed to launch thread");
            }

            let wait_result = WaitForSingleObject(thread, INFINITE);
            if wait_result == WAIT_FAILED {
                return to_library_error("Failed to wait for thread to complete");
            }

            let mut exit_code: u32 = 0;
            let library_handle = &mut exit_code as *mut u32;

            let get_exit_code_result = GetExitCodeThread(thread, exit_code_ptr);
            if get_exit_code_result == 0 {
                return to_library_error("Failed to get exit code for thread");
            }

            if library_handle == null_mut() {
                return to_library_error("Failed to LoadLibrary");
            }

            CloseHandle(thread);
        }

        Ok(())
    }

    unsafe fn to_library_error(msg: &str) -> Result<()> {
        return Err(super::ErrorKind::Library(format!("{}: {}", msg, GetLastError())));
    }
}
