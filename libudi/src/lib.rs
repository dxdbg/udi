// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]
#![recursion_limit = "1024"]

extern crate serde;
#[macro_use]
extern crate serde_derive;
#[macro_use]
extern crate downcast_rs;
#[macro_use]
extern crate derive_error_chain;

use std::fs;
use std::sync::{Mutex, Arc};

use downcast_rs::Downcast;

pub mod protocol;
mod errors;
mod create;
mod process;
mod thread;
mod events;

pub use errors::*;
pub use create::create_process;
pub use create::ProcessConfig;
pub use events::Event;
pub use events::wait_for_events;
pub use protocol::event::EventData;
pub use protocol::Architecture;
pub use protocol::Register;

pub trait UserData: Downcast + std::fmt::Debug {}
impl_downcast!(UserData);

#[derive(Debug)]
struct ProcessFileContext {
    request_file: fs::File,
    response_file: fs::File,
    events_file: fs::File
}

#[derive(Debug)]
pub struct Process {
    pid: u32,
    file_context: Option<ProcessFileContext>,
    architecture: Architecture,
    protocol_version: u32,
    multithread_capable: bool,
    running: bool,
    terminating: bool,
    user_data: Option<Box<UserData>>,
    threads: Vec<Arc<Mutex<Thread>>>,
    root_dir: String
}

#[derive(Debug,Copy,Clone,PartialEq)]
pub enum ThreadState {
    Running,
    Suspended
}

#[derive(Debug)]
struct ThreadFileContext {
    request_file: fs::File,
    response_file: fs::File
}

#[derive(Debug)]
pub struct Thread {
    initial: bool,
    tid: u64,
    file_context: Option<ThreadFileContext>,
    single_step: bool,
    state: ThreadState,
    architecture: Architecture,
    user_data: Option<Box<UserData>>
}
