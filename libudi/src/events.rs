//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use mio::{Events, Poll, Token};

use super::create::initialize_thread;
use super::errors::*;
use super::protocol::event::EventData;
use super::protocol::event::EventMessage;
use super::protocol::read_event;
use super::protocol::EventReadError;
use super::Process;
use super::Thread;

#[derive(Debug)]
pub struct Event {
    pub process: Arc<Mutex<Process>>,
    pub thread: Arc<Mutex<Thread>>,
    pub data: EventData,
}

pub fn wait_for_events(procs: &Vec<Arc<Mutex<Process>>>) -> Result<Vec<Event>, Error> {
    let mut output: Vec<Event> = vec![];

    for proc_ref in procs {
        let terminating;
        {
            let process = proc_ref.lock()?;
            terminating = process.terminating && process.running;
        }

        if terminating {
            // To work around an issue on macos with kqueue and fifos, if the process is
            // terminating, wait for the process to terminate, closing the event pipe.
            let event = handle_read_event(proc_ref)?;
            match event.data {
                EventData::ProcessCleanup => {
                    output.push(event);
                }
                _ => {
                    let msg = format!("Unexpected event {:?} for terminating process", event);
                    return Err(Error::Library(msg));
                }
            };
        }
    }

    let mut poll = Poll::new()?;
    let mut event_procs = HashMap::new();
    for proc_ref in procs {
        let mut process = proc_ref.lock()?;

        if !process.terminating && process.running {
            let token = Token(process.pid as usize);
            let file_context = process.get_file_context()?;
            sys::register_event_source(&poll, token, &file_context.events_file)?;

            event_procs.insert(token, proc_ref.clone());
        }
    }

    while output.is_empty() {
        let mut events = Events::with_capacity(procs.len());

        poll.poll(&mut events, None)?;

        for event in &events {
            if event.is_readable() {
                let event_token = event.token();
                if let Some(proc_ref) = event_procs.get(&event_token) {
                    let event = handle_read_event(proc_ref)?;
                    output.push(event);
                } else {
                    let msg = format!("Unknown event token {:?}", event_token);
                    return Err(Error::Library(msg));
                }
            }
        }
    }

    Ok(output)
}

fn handle_read_event(proc_ref: &Arc<Mutex<Process>>) -> Result<Event, Error> {
    let mut process = proc_ref.lock()?;

    match read_event(&mut process.get_file_context()?.events_file) {
        Ok(event_msg) => {
            let event = handle_event_message(proc_ref, &mut process, event_msg)?;

            Ok(event)
        }
        Err(EventReadError::Eof) => {
            // Process has closed its pipe
            process.file_context = None;

            Ok(Event {
                process: proc_ref.clone(),
                thread: process.threads[0].clone(),
                data: EventData::ProcessCleanup,
            })
        }
        Err(EventReadError::Udi(e)) => Err(e),
    }
}

fn handle_event_message(
    proc_ref: &Arc<Mutex<Process>>,
    process: &mut Process,
    message: EventMessage,
) -> Result<Event, Error> {
    process.running = false;

    // Locate the event thread
    let mut t = None;
    for thr in &(process.threads) {
        if thr.lock()?.tid == message.tid {
            t = Some(thr.clone());
            break;
        }
    }

    let event = match t {
        Some(thr) => Event {
            process: proc_ref.clone(),
            thread: thr,
            data: message.data,
        },
        None => {
            let msg = format!("Failed to locate event thread with tid {:?}", message.tid);
            return Err(Error::Library(msg));
        }
    };

    match event.data {
        EventData::ThreadCreate { tid } => {
            initialize_thread(&mut *process, tid)?;
        }
        EventData::ThreadDeath => {
            // Close the handles maintained by the thread
            let mut thr = event.thread.lock()?;
            thr.file_context = None;
        }
        EventData::ProcessExit { .. } => {
            process.terminating = true;
        }
        _ => {}
    }

    Ok(event)
}

#[cfg(unix)]
mod sys {

    use std::fs;
    use std::os::unix::io::AsRawFd;

    use mio::unix::SourceFd;
    use mio::{Interest, Poll, Token};

    use crate::Error;

    pub fn register_event_source(poll: &Poll, token: Token, file: &fs::File) -> Result<(), Error> {
        Ok(poll
            .registry()
            .register(&mut SourceFd(&file.as_raw_fd()), token, Interest::READABLE)?)
    }
}

#[cfg(windows)]
mod sys {
    use std::fs;
    use std::io;

    use super::mio::event::Evented;
    use super::mio::{Poll, PollOpt, Ready, Token};

    pub fn is_event_source_failed(_ready: Ready) -> bool {
        false
    }

    pub fn add_platform_ready_flags(ready: Ready) -> Ready {
        ready
    }

    pub struct EventSource {}

    impl EventSource {
        pub fn new(_file: &fs::File) -> EventSource {
            EventSource {}
        }
    }

    impl Evented for EventSource {
        fn register(
            &self,
            _poll: &Poll,
            _token: Token,
            _interest: Ready,
            _opts: PollOpt,
        ) -> io::Result<()> {
            Ok(())
        }

        fn reregister(
            &self,
            _poll: &Poll,
            _token: Token,
            _interest: Ready,
            _opts: PollOpt,
        ) -> io::Result<()> {
            Ok(())
        }

        fn deregister(&self, _poll: &Poll) -> io::Result<()> {
            Ok(())
        }
    }
}
