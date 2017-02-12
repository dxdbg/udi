//
// Copyright (c) 2011-2017, UDI Contributors
// All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#![deny(warnings)]
#![allow(unused_variables)]

use ::std::fs::File;
use ::std::io::Write;

use super::errors::*;
use super::Thread;
use super::protocol::Register;
use super::protocol::request;
use super::protocol::response;
use super::protocol::Architecture;
use super::UserData;

use serde::de::DeserializeOwned;
use serde::ser::Serialize;

impl Thread {

    pub fn set_user_data(&mut self, user_data: Box<UserData>) {
        self.user_data = Some(user_data);
    }

    pub fn get_user_data(&mut self) -> Option<&mut Box<UserData>> {
        self.user_data.as_mut()
    }

    pub fn get_tid(&self) -> u64 {
        self.tid
    }

    pub fn get_state(&self) -> super::ThreadState {
        self.state
    }

    pub fn get_pc(&mut self) -> Result<u64> {
        let reg = match self.architecture {
            Architecture::X86 => Register::X86_EIP,
            Architecture::X86_64 => Register::X86_64_RIP
        };

        self.read_register(reg)
    }

    pub fn set_single_step(&mut self, setting: bool) -> Result<()> {
        let msg = request::SingleStep::new(setting);

        let resp: response::SingleStep = self.send_request(&msg)?;

        self.single_step = setting;

        Ok(())
    }

    pub fn get_single_step(&self) -> bool {
        self.single_step
    }

    pub fn get_next_instruction(&mut self) -> Result<u64> {
        let msg = request::NextInstruction::new();

        let resp: response::NextInstruction = self.send_request(&msg)?;

        Ok(resp.addr)
    }

    pub fn suspend(&mut self) -> Result<()> {
        let msg = request::ThreadSuspend::new();

        self.send_request_no_data(&msg)?;

        Ok(())
    }

    pub fn resume(&mut self) -> Result<()> {
        let msg = request::ThreadResume::new();

        self.send_request_no_data(&msg)?;
        Ok(())
    }

    pub fn read_register(&mut self, reg: Register) -> Result<u64> {
        let msg = request::ReadRegister::new(reg as u32);

        let resp: response::ReadRegister = self.send_request(&msg)?;

        Ok(resp.value)
    }

    pub fn write_register(&mut self, reg: Register, value: u64) -> Result<()> {
        let msg = request::WriteRegister::new(reg as u32, value);

        self.send_request_no_data(&msg)?;

        Ok(())
    }

    fn send_request<T: DeserializeOwned, S: request::RequestType + Serialize>(&mut self, msg: &S)
            -> Result<T> {
        let ctx = match self.file_context.as_mut() {
            Some(ctx) => ctx,
            None => {
                let msg = format!("Thread {:?} terminated, cannot performed requested operation",
                                  self.tid);
                return Err(ErrorKind::Request(msg).into());
            }
        };

        ctx.request_file.write_all(&request::serialize(msg)?)?;

        response::read::<T, File>(&mut ctx.response_file)
    }

    fn send_request_no_data<S: request::RequestType + Serialize>(&mut self, msg: &S)
            -> Result<()> {
        let ctx = match self.file_context.as_mut() {
            Some(ctx) => ctx,
            None => {
                let msg = format!("Thread {:?} terminated, cannot performed requested operation",
                                  self.tid);
                return Err(ErrorKind::Request(msg).into());
            }
        };

        ctx.request_file.write_all(&request::serialize(msg)?)?;

        response::read_no_data::<File>(&mut ctx.response_file)
    }
}

impl PartialEq for Thread {
    fn eq(&self, other: &Thread) -> bool {
        self.tid == other.tid
    }
}
