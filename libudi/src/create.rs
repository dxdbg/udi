//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

use std::string::String;
use std::sync::{Mutex, Arc};
use std::io::Write;
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

pub use self::sys::UdiChild;

const REQUEST_FILE_NAME: &'static str = "request";
const RESPONSE_FILE_NAME: &'static str = "response";
const EVENTS_FILE_NAME: &'static str = "events";

#[derive(Debug)]
pub struct ProcessConfig {
    root_dir: Option<String>,
    rt_lib_path: Option<String>
}

pub(crate) struct HandshakeData {
    file_context: ProcessFileContext,
    init: response::Init
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

    let child = sys::launch_process(executable,
                                        argv,
                                        envp,
                                        config)?;

    let process = initialize_process(child)?;

    Ok(Arc::new(Mutex::new(process)))
}

fn initialize_process(mut child: UdiChild) -> Result<Process> {

    let handshake_data = child.process_handshake()?;
    let init = handshake_data.init;

    let version = determine_protocol(&init)?;
    let pid = child.id();

    let mut process = Process{
        pid,
        file_context: Some(handshake_data.file_context),
        architecture: init.arch,
        protocol_version: version,
        multithread_capable: init.mt,
        running: false,
        terminating: false,
        user_data: None,
        threads: vec![],
        child
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
pub fn initialize_thread(process: &mut Process,
                         tid: u64) -> Result<()> {

    let request_path = process.child.thr_request_path(tid);
    let response_path = process.child.thr_response_path(tid);

    let mut request_file = fs::OpenOptions::new().read(false)
                                                 .create(false)
                                                 .write(true)
                                                 .open(request_path)?;

    request_file.write_all(&protocol::request::serialize(&request::Init::new())?)?;

    let mut response_file = fs::File::open(response_path)?;

    protocol::response::read::<response::Init, _>(&mut response_file)?;

    let thr = Thread{
        initial: process.threads.len() == 0,
        tid,
        file_context: Some(ThreadFileContext{ request_file, response_file }),
        single_step: false,
        state: ThreadState::Running,
        architecture: process.architecture,
        user_data: None
    };

    process.threads.push(Arc::new(Mutex::new(thr)));

    Ok(())
}

#[cfg(any(target_os = "macos", target_os = "linux"))]
mod sys {
    extern crate users;

    use ::std::path::PathBuf;
    use ::std::fs;
    use ::std::io::{self,Write};
    use super::request;
    use super::response;
    use super::protocol;
    use super::ProcessFileContext;

    const UDI_ROOT_DIR_ENV: &'static str = "UDI_ROOT_DIR";
    const DEFAULT_UDI_RT_LIB_NAME: &'static str = "libudirt.so";

    pub struct UdiChild {
        child: ::std::process::Child,
        root_dir: PathBuf
    }

    impl ::std::fmt::Debug for UdiChild {
        fn fmt(&self, f: & mut ::std::fmt::Formatter) -> ::std::fmt::Result {
            write!(f, "UdiChild {{ }}")
        }
    }

    impl ::std::fmt::Display for UdiChild {
        fn fmt(&self, f: & mut ::std::fmt::Formatter) -> ::std::fmt::Result {
            write!(f, "{{ }}")
        }
    }

    impl UdiChild {

        pub fn id(&self) -> u32 {
            self.child.id()
        }

        fn try_wait(&mut self) -> super::Result<()> {
            match self.child.try_wait()? {
                Some(status) => {
                    let msg = format!("Process failed to initialize, exited with status: {}",
                                      status);
                    Err(super::ErrorKind::Library(msg).into())
                },
                None => Ok(())
            }
        }

        pub(crate) fn process_handshake(&mut self) -> super::Result<super::HandshakeData> {

            let request_path = self.request_path();
            let response_path = self.response_path();
            let events_path = self.events_path();

            // poll for change in root UDI filesystem
            let mut event_file_exists = false;
            while !event_file_exists {
                self.try_wait()?;

                match fs::metadata(&events_path) {
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
            let events_file = fs::File::open(events_path)?;

            let init: response::Init = protocol::response::read(&mut response_file)?;

            Ok(super::HandshakeData{
                file_context: ProcessFileContext{ request_file, response_file, events_file },
                init
            })
        }

        pub fn request_path(&self) -> PathBuf {
            let mut request_path_buf = self.root_dir.clone();
            request_path_buf.push(super::REQUEST_FILE_NAME);
            request_path_buf
        }

        pub fn thr_request_path(&self, tid: u64) -> PathBuf {
            let mut request_path_buf = self.root_dir.clone();
            request_path_buf.push(format!("{:016x}", tid));
            request_path_buf.push(super::REQUEST_FILE_NAME);
            request_path_buf
        }

        pub fn response_path(&self) -> PathBuf {
            let mut response_path_buf = self.root_dir.clone();
            response_path_buf.push(super::RESPONSE_FILE_NAME);
            response_path_buf
        }

        pub fn thr_response_path(&self, tid: u64) -> PathBuf {
            let mut response_path_buf = self.root_dir.clone();
            response_path_buf.push(format!("{:016x}", tid));
            response_path_buf.push(super::RESPONSE_FILE_NAME);
            response_path_buf
        }

        pub fn events_path(&self) -> PathBuf {
            let mut events_path_buf = self.root_dir.clone();
            events_path_buf.push(super::EVENTS_FILE_NAME);
            events_path_buf
        }
    }

    fn get_current_username() -> super::Result<String> {

        match users::get_current_username() {
            Some(user) => Ok(user),
            None => {
                Err(super::ErrorKind::Library("Failed to retrieve username".to_owned()).into())
            }
        }
    }

    pub fn launch_process(executable: &str,
                          argv: &Vec<String>,
                          envp: &Vec<String>,
                          config: &super::ProcessConfig) -> super::Result<UdiChild> {

        let rt_lib_path = config.rt_lib_path
                                .clone()
                                .unwrap_or(DEFAULT_UDI_RT_LIB_NAME.to_owned());

        let root_dir = create_root_udi_filesystem(config)?;

        let mut command = ::std::process::Command::new(executable);

        let env = create_environment(envp,
                                     &root_dir,
                                     &rt_lib_path);

        for entry in env {
            command.env(entry.0, entry.1);
        }

        for entry in argv {
            command.arg(entry);
        }

        let child = command.spawn()?;

        let mut root_dir_buf = PathBuf::from(root_dir);
        root_dir_buf.push(child.id().to_string());

        Ok(UdiChild{child, root_dir: root_dir_buf})
    }

    fn create_root_udi_filesystem(config: &super::ProcessConfig) -> super::Result<String> {

        let root_dir = config.root_dir.clone().map_or(::std::env::temp_dir(),
                                                      |d| ::std::path::Path::new(&d).to_owned());

        mkdir_ignore_exists(&root_dir)?;

        let user = get_current_username()?;

        let mut user_dir_path = root_dir.to_owned();
        user_dir_path.push(user);

        mkdir_ignore_exists(user_dir_path.as_path())?;

        if let Some(user_dir_path_str) = user_dir_path.to_str() {
            Ok(user_dir_path_str.to_owned())
        } else {
            Err(super::ErrorKind::Library("Invalid root UDI filesystem path".to_owned()).into())
        }
    }

    fn mkdir_ignore_exists(dir: &::std::path::Path) -> super::Result<()> {
        match ::std::fs::create_dir(dir) {
            Ok(_) => Ok(()),
            Err(e) => match e.kind() {
                ::std::io::ErrorKind::AlreadyExists => Ok(()),
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

        let var_name = get_dyn_linker_var_name();

        // Update dynamic linker variable to include the runtime library
        let mut updated_var = false;
        for mut item in &mut output {
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

        modify_env(&mut output);

        output
    }

    #[cfg(target_os = "linux")]
    fn get_dyn_linker_var_name() -> &'static str {
        "LD_PRELOAD"
    }

    #[cfg(target_os = "macos")]
    fn get_dyn_linker_var_name() -> &'static str {
        "DYLD_INSERT_LIBRARIES"
    }

    #[cfg(target_os = "linux")]
    fn modify_env(_env: &mut Vec<(String, String)>) {
    }

    #[cfg(target_os = "macos")]
    fn modify_env(env: &mut Vec<(String, String)>) {
        env.push(("DYLD_FORCE_FLAT_NAMESPACE".to_owned(), "1".to_owned()));
    }
}

#[cfg(windows)]
mod sys {
    extern crate winapi;

    use ::std::ptr::null_mut;
    use ::std::ffi::CString;
    use ::std::mem::zeroed;
    use ::std::mem::size_of;
    use ::std::path::PathBuf;

    use create::sys::winapi::um::memoryapi::{
        VirtualAllocEx,
        VirtualFreeEx,
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
        HANDLE,
        PAGE_READWRITE,
        MEM_COMMIT,
        MEM_RELEASE
    };
    use create::sys::winapi::um::winbase::{
        CREATE_SUSPENDED,
        INFINITE,
        WAIT_FAILED,
        WAIT_OBJECT_0
    };
    use create::sys::winapi::um::processthreadsapi::{
        PROCESS_INFORMATION,
        LPPROCESS_INFORMATION,
        STARTUPINFOA,
        LPSTARTUPINFOA,
        CreateRemoteThread,
        GetExitCodeThread,
        GetExitCodeProcess,
        ResumeThread,
        CreateProcessA
    };
    use create::sys::winapi::um::synchapi::{
        WaitForSingleObject
    };
    use create::sys::winapi::um::handleapi::{
        INVALID_HANDLE_VALUE,
        CloseHandle
    };
    use create::sys::winapi::shared::minwindef::{
        FALSE
    };
    use create::sys::winapi::shared::winerror::{
        WAIT_TIMEOUT
    };

    use create::sys::winapi::ctypes::c_void;

    const DEFAULT_UDI_RT_LIB_NAME: &'static str = "udirt.dll";
    const PIPE_NAME_BASE: &'static str = "\\\\.\\pipe\\udi";

    pub struct UdiChild {
        proc_info: PROCESS_INFORMATION
    }

    impl ::std::fmt::Debug for UdiChild {
        fn fmt(&self, f: & mut ::std::fmt::Formatter) -> ::std::fmt::Result {
            write!(f, "UdiChild {{ }}")
        }
    }

    impl ::std::fmt::Display for UdiChild {
        fn fmt(&self, f: & mut ::std::fmt::Formatter) -> ::std::fmt::Result {
            write!(f, "{{ }}")
        }
    }

    impl UdiChild {

        pub fn id(&self) -> u32 {
            self.proc_info.dwProcessId
        }

        fn try_wait(&mut self) -> super::Result<()> {
            unsafe {
                let wait_result = WaitForSingleObject(self.proc_info.hProcess, 0);
                if wait_result == WAIT_TIMEOUT {
                    return Ok(());
                }
                if wait_result == WAIT_OBJECT_0 {
                    let status = self.get_exit_code()?;
                    let msg = format!("Process failed to initialize, exited with status: {}",
                                      status);
                    return Err(super::ErrorKind::Library(msg).into());
                }

                to_library_error("Failed to wait for process")
            }
        }

        pub(crate) fn process_handshake(&mut self) -> super::Result<super::HandshakeData> {

            // TODO here
            let request_path = self.request_path();
            let response_path = self.response_path();
            let events_path = self.events_path();

            // poll for change in root UDI filesystem
            let mut event_file_exists = false;
            while !event_file_exists {
                self.try_wait()?;

                match fs::metadata(&events_path) {
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
            let events_file = fs::File::open(events_path)?;

            let init: response::Init = protocol::response::read(&mut response_file)?;

            Ok(super::HandshakeData{
                file_context: ProcessFileContext{ request_file, response_file, events_file },
                init
            })
        }

        pub fn request_path(&self) -> PathBuf {
            let request_path = format!("{}-{}-{}",
                                       PIPE_NAME_BASE,
                                       self.id(),
                                       super::REQUEST_FILE_NAME);
            PathBuf::from(request_path)
        }

        pub fn thr_request_path(&self, tid: u64) -> PathBuf {
            let request_path = format!("{}-{}-{}-{}",
                                       PIPE_NAME_BASE,
                                       self.id(),
                                       tid,
                                       super::REQUEST_FILE_NAME);
            PathBuf::from(request_path)
        }

        pub fn response_path(&self) -> PathBuf {
            let response_path = format!("{}-{}-{}",
                                        PIPE_NAME_BASE,
                                        self.id(),
                                        super::RESPONSE_FILE_NAME);
            PathBuf::from(response_path)
        }

        pub fn thr_response_path(&self, tid: u64) -> PathBuf {
            let response_path = format!("{}-{}-{}-{}",
                                        PIPE_NAME_BASE,
                                        self.id(),
                                        tid,
                                        super::RESPONSE_FILE_NAME);
            PathBuf::from(response_path)
        }

        pub fn events_path(&self) -> PathBuf {
            let events_path = format!("{}-{}-{}",
                                      PIPE_NAME_BASE,
                                      self.id(),
                                      super::EVENTS_FILE_NAME);
            PathBuf::from(events_path)
        }

        fn get_exit_code(&mut self) -> super::Result<u32> {
            unsafe {
                let mut exit_code: u32 = 0;
                let exit_code_ptr = &mut exit_code as *mut u32;

                let get_exit_code_result = GetExitCodeProcess(self.proc_info.hProcess,
                                                              exit_code_ptr);
                if get_exit_code_result == FALSE {
                    to_library_error("Failed to exit code for process")?;
                }

                Ok(exit_code)
            }
        }
    }

    impl Drop for UdiChild {
        fn drop(&mut self) {
            unsafe {
                CloseHandle(self.proc_info.hThread);
                CloseHandle(self.proc_info.hProcess);
            }
        }
    }

    pub fn launch_process(executable: &str,
                          argv: &Vec<String>,
                          envp: &Vec<String>,
                          config: &super::ProcessConfig) -> super::Result<UdiChild> {

        let rt_lib_path = config.rt_lib_path
            .clone()
            .unwrap_or(DEFAULT_UDI_RT_LIB_NAME.to_owned());

        let proc_info = unsafe {

            let proc_info = create_win_process(executable, argv, envp)?;

            let library_handle_value = apply_remote_function(proc_info.hProcess,
                                                             "KERNEL32.DLL",
                                                             "LoadLibraryA",
                                                             Some(&rt_lib_path))?;

            if library_handle_value == (-1i32 as u32) {
                let library_load_exit_code = apply_remote_function(proc_info.hProcess,
                                                                   "KERNEL32.DLL",
                                                                   "GetLastError",
                                                                   None)?;
                return Err(super::ErrorKind::Library(format!("Failed to load library {}: {}",
                                                             rt_lib_path,
                                                             library_load_exit_code)).into());
            }

            let resume_result = ResumeThread(proc_info.hThread);
            if resume_result == (-1i32 as u32) {
                to_library_error("Failed to resume thread")?;
            }

            proc_info
        };

        Ok(UdiChild{ proc_info })
    }

    unsafe fn create_win_process(executable: &str,
                                 _argv: &Vec<String>,
                                 _envp: &Vec<String>) -> super::Result<PROCESS_INFORMATION> {

        let mut proc_info: PROCESS_INFORMATION = zeroed();
        let mut startup_info: STARTUPINFOA = zeroed();

        startup_info.cb = size_of::<STARTUPINFOA>() as u32;

        let exec_path_str = CString::new(executable)?;

        // TODO
        // - use CreateProcessW, converting to wide strings
        // - implement handling for argv, envp
        let create_result = CreateProcessA(exec_path_str.as_ptr(),
                                           null_mut(),
                                           null_mut(),
                                           null_mut(),
                                           FALSE,
                                           CREATE_SUSPENDED,
                                           null_mut(),
                                           null_mut(),
                                           &mut startup_info as LPSTARTUPINFOA,
                                           &mut proc_info as LPPROCESS_INFORMATION);
        if create_result == FALSE {
            println!("{:?}", exec_path_str);
            to_library_error("Failed to create process")?;
        }

        Ok(proc_info)
    }

    struct RemoteMemory {
        handle: HANDLE,
        addr: *mut c_void,
        len: usize
    }

    impl RemoteMemory {
        pub unsafe fn new(handle: HANDLE, data: &[u8]) -> super::Result<RemoteMemory> {
            let len = data.len();

            let addr = VirtualAllocEx(handle,
                                      null_mut(),
                                      len,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);

            if addr == null_mut() {
                to_library_error("Failed to allocate memory")?;
            }

            let write_result = WriteProcessMemory(handle,
                                                  addr,
                                                  data.as_ptr() as *const c_void,
                                                  len,
                                                  null_mut());

            if write_result == FALSE {
                VirtualFreeEx(handle, addr, len, MEM_RELEASE);
                to_library_error("Failed to write process memory")?;
            }

            Ok(RemoteMemory{
                handle,
                addr,
                len
            })
        }

        pub fn empty() -> RemoteMemory {
            RemoteMemory{
                handle: INVALID_HANDLE_VALUE,
                addr: null_mut(),
                len: 0
            }
        }

        pub fn addr(&self) -> *mut c_void {
            self.addr
        }
    }

    impl Drop for RemoteMemory {
        fn drop(&mut self) {
            unsafe {
                if self.handle != INVALID_HANDLE_VALUE {
                    VirtualFreeEx(self.handle,
                                  self.addr,
                                  self.len,
                                  MEM_RELEASE);
                }
            }
        }
    }

    unsafe fn apply_remote_function(handle: HANDLE,
                                    module: &str,
                                    function_name: &str,
                                    parameter: Option<&str>) -> super::Result<u32> {

        let module_str = CString::new(module)?;
        let module_handle = GetModuleHandleA(module_str.as_ptr());
        if module_handle == null_mut() {
            to_library_error(&format!("Failed to get handle to {}", module))?;
        }

        let function_name_str = CString::new(function_name)?;
        let function_address_result = GetProcAddress(module_handle,
                                                     function_name_str.as_ptr());
        let function_ptr: unsafe extern "system" fn(*mut c_void) -> u32 =
            ::std::mem::transmute(function_address_result as *const c_void);

        let remote_parameter = match parameter {
            Some(p) => {
                let parameter_str = CString::new(p)?;

                RemoteMemory::new(handle, parameter_str.as_bytes_with_nul())?
            },
            None => RemoteMemory::empty()
        };

        let thread = CreateRemoteThread(handle,
                                        null_mut(),
                                        0,
                                        Some(function_ptr),
                                        remote_parameter.addr(),
                                        0,
                                        null_mut());
        if thread == INVALID_HANDLE_VALUE {
            to_library_error("Failed to launch thread")?;
        }

        let wait_result = WaitForSingleObject(thread, INFINITE);
        if wait_result == WAIT_FAILED {
            CloseHandle(thread);
            to_library_error("Failed to wait for thread to complete")?;
        }

        let mut exit_code: u32 = 0;
        let exit_code_ptr = &mut exit_code as *mut u32;

        let get_exit_code_result = GetExitCodeThread(thread, exit_code_ptr);
        if get_exit_code_result == FALSE {
            CloseHandle(thread);
            to_library_error("Failed to get exit code for thread")?;
        }

        CloseHandle(thread);

        Ok(exit_code)
    }

    unsafe fn to_library_error(msg: &str) -> super::Result<()> {
        return Err(super::ErrorKind::Library(format!("{}: {}", msg, GetLastError())).into());
    }

    impl<'a> From<::std::ffi::NulError> for super::Error {
        fn from(err: ::std::ffi::NulError) -> super::Error {
            let msg = format!("{}", err);
            super::ErrorKind::Library(msg).into()
        }
    }
}
