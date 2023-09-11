//
// Copyright (c) 2011-2023, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

use std::io;
use std::sync;

use thiserror::Error;

use super::Process;
use super::Thread;

#[derive(Debug, Error)]
pub enum Error {
    #[error("UDI I/O error")]
    Io(#[from] io::Error),

    #[error("Library error: {0}")]
    Library(String),

    #[error("Invalid request: {0}")]
    Request(String),
}

impl<'a> From<sync::PoisonError<sync::MutexGuard<'a, Process>>> for Error {
    fn from(err: sync::PoisonError<sync::MutexGuard<'a, Process>>) -> Error {
        Error::Library(format!("{}", err))
    }
}

impl<'a> From<sync::PoisonError<sync::MutexGuard<'a, Thread>>> for Error {
    fn from(err: sync::PoisonError<sync::MutexGuard<'a, Thread>>) -> Error {
        Error::Library(format!("{}", err))
    }
}

impl From<ciborium::de::Error<std::io::Error>> for Error {
    fn from(err: ciborium::de::Error<std::io::Error>) -> Error {
        match err {
            ciborium::de::Error::Io(err) => Error::Io(err),
            _ => Error::Library(format!("{}", err)),
        }
    }
}
