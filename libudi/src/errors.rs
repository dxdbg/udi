//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]

extern crate serde_cbor;

use std::error::Error as StdError;
use std::io;
use std::sync;

use super::Process;
use super::Thread;

#[derive(Debug, ErrorChain)]
pub enum ErrorKind {
    #[error_chain(foreign)]
    Io(io::Error),

    #[error_chain(foreign)]
    #[error_chain(display = "cbor_error_display")]
    Cbor(serde_cbor::Error),

    #[error_chain(custom)]
    #[error_chain(description = "library_error_description")]
    #[error_chain(display = "library_error_display")]
    Library(String),

    #[error_chain(custom)]
    #[error_chain(description = "request_error_description")]
    #[error_chain(display = "request_error_display")]
    Request(String)
}

fn cbor_error_display(f: &mut ::std::fmt::Formatter, e: &serde_cbor::Error)
    -> ::std::fmt::Result {

    match *e {
        serde_cbor::Error::Custom(ref s) => write!(f, "CBOR error: {}", s),
        _ => write!(f, "CBOR error: {}", e.description())
    }
}

fn library_error_description(_: &str) -> &str {
    "library error"
}

fn library_error_display(f: &mut ::std::fmt::Formatter, s: &str)
    -> ::std::fmt::Result {

    write!(f, "library error: {}", s)
}

fn request_error_description(_: &str) -> &str {
    "invalid request"
}

fn request_error_display(f: &mut ::std::fmt::Formatter, s: &str)
    -> ::std::fmt::Result {

    write!(f, "invalid request: {}", s)
}

impl<'a> From<sync::PoisonError<sync::MutexGuard<'a, Process>>> for Error {
    fn from(err: sync::PoisonError<sync::MutexGuard<'a, Process>>) -> Error {
        ErrorKind::Library(err.description().to_owned()).into()
    }
}

impl<'a> From<sync::PoisonError<sync::MutexGuard<'a, Thread>>> for Error {
    fn from(err: sync::PoisonError<sync::MutexGuard<'a, Thread>>) -> Error {
        ErrorKind::Library(err.description().to_owned()).into()
    }
}
