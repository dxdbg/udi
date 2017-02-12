//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

extern crate mio;

use std::sync::{Mutex,Arc};
use std::collections::HashMap;

use self::mio::{Poll, Events, Ready, PollOpt, Token};

use super::errors::*;
use super::Process;
use super::Thread;
use super::create::initialize_thread;
use super::protocol::event::EventData;
use super::protocol::event::EventMessage;
use super::protocol::read_event;
use super::protocol::EventReadError;

#[derive(Debug)]
pub struct Event {
    pub process: Arc<Mutex<Process>>,
    pub thread: Arc<Mutex<Thread>>,
    pub data: EventData
}

pub fn wait_for_events(procs: &Vec<Arc<Mutex<Process>>>) -> Result<Vec<Event>> {

    let poll = Poll::new()?;
    let mut event_procs = HashMap::new();
    for proc_ref in procs {

        let mut process = proc_ref.lock()?;

        let token = Token(process.pid as usize);
        {
            let file_context = process.get_file_context()?;

            let event_source = sys::EventSource::new(&file_context.events_file);

            poll.register(&event_source, token, Ready::readable(), PollOpt::edge())?;
        }

        event_procs.insert(token, proc_ref.clone());
    }

    let mut output: Vec<Event> = vec![];
    while output.len() == 0 {

        let mut events = Events::with_capacity(procs.len());

        poll.poll(&mut events, None)?;

        for event in &events {
            let ready = event.readiness();
            if ready.is_readable() || sys::is_event_source_failed(ready) {
                let event_token = event.token();
                if let Some(proc_ref) = event_procs.get(&event_token) {
                    let event = handle_read_event(&proc_ref)?;
                    output.push(event);
                }else{
                    let msg = format!("Unknown event token {:?}", event_token);
                    return Err(Error::from_kind(ErrorKind::Library(msg)));
                }
            }
        }
    }

    Ok(output)
}

fn handle_read_event(proc_ref: &Arc<Mutex<Process>>) -> Result<Event> {

    let mut process = proc_ref.lock()?;

    match read_event(&mut process.get_file_context()?.events_file) {
        Ok(event_msg) => {
            let event = handle_event_message(&proc_ref,
                                             &mut *process,
                                             event_msg)?;

            Ok(event)
        },
        Err(EventReadError::Eof) => {
            // Process has closed its pipe
            process.file_context = None;

            Ok(Event{
                process: proc_ref.clone(),
                thread: process.threads[0].clone(),
                data: EventData::ProcessCleanup
            })
        },
        Err(EventReadError::Udi(e)) => Err(e)
    }
}

fn handle_event_message(proc_ref: &Arc<Mutex<Process>>,
                        process: &mut Process,
                        message: EventMessage) -> Result<Event> {

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
        Some(thr) => {
            Event{
                process: proc_ref.clone(),
                thread: thr,
                data: message.data
            }
        },
        None => {
            let msg = format!("Failed to locate event thread with tid {:?}", message.tid);
            return Err(Error::from_kind(ErrorKind::Library(msg)));
        }
    };

    match event.data {
        EventData::ThreadCreate{ tid } => {
            initialize_thread(&mut *process, tid)?;
        },
        EventData::ThreadDeath => {
            // Close the handles maintained by the thread
            let mut thr = event.thread.lock()?;
            thr.file_context = None;
        },
        EventData::ProcessExit{ .. } => {
            process.terminating = true;
        }
        _ => {}
    }

    Ok(event)
}

#[cfg(unix)]
mod sys {

    use std::os::unix::io::RawFd;
    use std::os::unix::io::AsRawFd;
    use std::io;
    use std::fs;

    use super::mio::{Ready, Poll, PollOpt, Token};
    use super::mio::unix::EventedFd;
    use super::mio::event::Evented;
    use super::mio::unix::UnixReady;

    pub fn is_event_source_failed(ready: Ready) -> bool {
        let unix_ready = UnixReady::from(ready);
        unix_ready.is_hup() || unix_ready.is_error()
    }

    pub struct EventSource {
        fd: RawFd
    }

    impl EventSource {
        pub fn new(file: &fs::File) -> EventSource {
            EventSource{ fd: file.as_raw_fd() }
        }
    }

    impl Evented for EventSource {
        fn register(&self, poll: &Poll, token: Token, interest: Ready, opts: PollOpt)
            -> io::Result<()> {

            EventedFd(&self.fd).register(poll, token, interest, opts)
        }

        fn reregister(&self, poll: &Poll, token: Token, interest: Ready, opts: PollOpt)
            -> io::Result<()> {

            EventedFd(&self.fd).reregister(poll, token, interest, opts)
        }

        fn deregister(&self, poll: &Poll) -> io::Result<()> {
            EventedFd(&self.fd).deregister(poll)
        }
    }
}
