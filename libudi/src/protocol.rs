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

extern crate serde_cbor;

use std::io;

use serde::de::{Deserialize, DeserializeOwned};
use serde::ser::Serialize;
use self::serde_cbor::de::Deserializer;

use super::errors::*;

pub const UDI_PROTOCOL_VERSION_1: u32 = 1;

// From serde documentation
macro_rules! enum_number {
    ($name:ident { $($variant:ident = $value:expr, )* }) => {
        #[repr(C)]
        #[derive(Clone, Copy, Debug, Eq, PartialEq)]
        pub enum $name {
            $($variant = $value,)*
        }

        impl ::serde::Serialize for $name {
            fn serialize<S>(&self, serializer: S) -> ::std::result::Result<S::Ok, S::Error>
                where S: ::serde::Serializer
            {
                // Serialize the enum as a u16.
                serializer.serialize_u16(*self as u16)
            }
        }

        impl<'de> ::serde::Deserialize<'de> for $name {
            fn deserialize<D>(deserializer: D) -> ::std::result::Result<Self, D::Error>
                where D: ::serde::Deserializer<'de>
            {
                struct Visitor;

                impl<'de> ::serde::de::Visitor<'de> for Visitor {
                    type Value = $name;

                    fn expecting(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
                        formatter.write_str("positive integer")
                    }

                    fn visit_u16<E>(self, value: u16) -> ::std::result::Result<$name, E>
                        where E: ::serde::de::Error
                    {
                        // Rust does not come with a simple way of converting a
                        // number to an enum, so use a big `match`.
                        match value {
                            $( $value => Ok($name::$variant), )*
                            _ => Err(E::custom(
                                format!("unknown {} value: {}",
                                stringify!($name), value))),
                        }
                    }
                }

                // Deserialize the enum from a u16.
                deserializer.deserialize_u16(Visitor)
            }
        }
    }
}

enum_number!(Architecture {
    X86 = 0,
    X86_64 = 1,
});

pub mod request {

    enum_number!(Type {
        Invalid = 0,
        Continue = 1,
        ReadMemory = 2,
        WriteMemory = 3,
        ReadRegister = 4,
        WriteRegister = 5,
        State = 6,
        Init = 7,
        CreateBreakpoint = 8,
        InstallBreakpoint = 9,
        RemoveBreakpoint = 10,
        DeleteBreakpoint = 11,
        ThreadSuspend = 12,
        ThreadResume = 13,
        NextInstruction = 14,
        SingleStep = 15,
    });

    pub trait RequestType {
        fn typ(&self) -> Type;

        fn empty(&self) -> bool {
            false
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct Init {
        #[serde(skip_serializing)]
        typ: Type,
    }

    impl Init {
        pub fn new() -> Init {
            Init{ typ: Type::Init }
        }
    }

    impl RequestType for Init {
        fn typ(&self) -> Type {
            self.typ
        }

        fn empty(&self) -> bool {
            true
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct Continue {
        #[serde(skip_serializing)]
        typ: Type,
        pub sig: u32
    }

    impl Continue {
        pub fn new(sig: u32) -> Continue {
            Continue{ typ: Type::Continue, sig: sig }
        }
    }

    impl RequestType for Continue {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct ReadMemory {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64,
        pub len: u32
    }

    impl ReadMemory {
        pub fn new(addr: u64, len: u32) -> ReadMemory {
            ReadMemory{ typ: Type::ReadMemory, addr, len }
        }
    }

    impl RequestType for ReadMemory {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct WriteMemory<'a> {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64,
        pub data: &'a[u8]
    }

    impl<'a> WriteMemory<'a> {
        pub fn new(addr: u64, data: &'a[u8]) -> WriteMemory {
            WriteMemory{ typ: Type::WriteMemory, addr, data }
        }
    }

    impl<'a> RequestType for WriteMemory<'a> {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct ReadRegister {
        #[serde(skip_serializing)]
        typ: Type,
        pub reg: u32
    }

    impl ReadRegister {
        pub fn new(reg: u32) -> ReadRegister {
            ReadRegister{ typ: Type::ReadRegister, reg }
        }
    }

    impl RequestType for ReadRegister {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct WriteRegister {
        #[serde(skip_serializing)]
        typ: Type,
        pub reg: u32,
        pub value: u64
    }

    impl WriteRegister {
        pub fn new(reg: u32, value: u64) -> WriteRegister {
            WriteRegister{ typ: Type::WriteRegister, reg, value }
        }
    }

    impl RequestType for WriteRegister {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct CreateBreakpoint {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64
    }

    impl CreateBreakpoint {
        pub fn new(addr: u64) -> CreateBreakpoint {
            CreateBreakpoint{ typ: Type::CreateBreakpoint, addr: addr }
        }
    }

    impl RequestType for CreateBreakpoint {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct InstallBreakpoint {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64
    }

    impl InstallBreakpoint {
        pub fn new(addr: u64) -> InstallBreakpoint {
            InstallBreakpoint{ typ: Type::InstallBreakpoint, addr: addr }
        }
    }

    impl RequestType for InstallBreakpoint {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct RemoveBreakpoint {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64
    }

    impl RemoveBreakpoint {
        pub fn new(addr: u64) -> RemoveBreakpoint {
            RemoveBreakpoint{ typ: Type::RemoveBreakpoint, addr: addr }
        }
    }

    impl RequestType for RemoveBreakpoint {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct DeleteBreakpoint {
        #[serde(skip_serializing)]
        typ: Type,
        pub addr: u64
    }

    impl DeleteBreakpoint {
        pub fn new(addr: u64) -> DeleteBreakpoint {
            DeleteBreakpoint{ typ: Type::DeleteBreakpoint, addr: addr }
        }
    }

    impl RequestType for DeleteBreakpoint {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct ThreadSuspend {
        #[serde(skip_serializing)]
        typ: Type
    }

    impl ThreadSuspend {
        pub fn new() -> ThreadSuspend {
            ThreadSuspend{ typ: Type::ThreadSuspend }
        }
    }

    impl RequestType for ThreadSuspend {
        fn typ(&self) -> Type {
            self.typ
        }

        fn empty(&self) -> bool {
            true
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct ThreadResume {
        #[serde(skip_serializing)]
        typ: Type
    }

    impl ThreadResume {
        pub fn new() -> ThreadResume {
            ThreadResume{ typ: Type::ThreadResume }
        }
    }

    impl RequestType for ThreadResume {
        fn typ(&self) -> Type {
            self.typ
        }

        fn empty(&self) -> bool {
            true
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct State {
        #[serde(skip_serializing)]
        typ: Type
    }

    impl State {
        pub fn new() -> State {
            State{ typ: Type::State }
        }
    }

    impl RequestType for State {
        fn typ(&self) -> Type {
            self.typ
        }

        fn empty(&self) -> bool {
            true
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct NextInstruction {
        #[serde(skip_serializing)]
        typ: Type
    }

    impl NextInstruction {
        pub fn new() -> NextInstruction {
            NextInstruction{ typ: Type::NextInstruction }
        }
    }

    impl RequestType for NextInstruction {
        fn typ(&self) -> Type {
            self.typ
        }

        fn empty(&self) -> bool {
            true
        }
    }

    #[derive(Deserialize, Serialize, Debug)]
    pub struct SingleStep {
        #[serde(skip_serializing)]
        typ: Type,
        value: bool
    }

    impl SingleStep {
        pub fn new(setting: bool) -> SingleStep {
            let value = setting;

            SingleStep{ typ: Type::SingleStep, value }
        }
    }

    impl RequestType for SingleStep {
        fn typ(&self) -> Type {
            self.typ
        }
    }

    pub fn serialize<T: super::Serialize + RequestType>(req: &T)
            -> super::Result<Vec<u8>> {
        let mut output: Vec<u8> = Vec::new();

        super::serde_cbor::ser::to_writer(&mut output, &req.typ())?;
        if !req.empty() {
            super::serde_cbor::ser::to_writer(&mut output, req)?;
        }

        Ok(output)
    }
}

pub mod response {

    enum_number!(Type {
        Valid = 0,
        Error = 1,
    });

    #[derive(Deserialize,Serialize,Debug)]
    pub struct Init {
        pub v: u32,
        pub arch: super::Architecture,
        pub mt: bool,
        pub tid: u64
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ReadMemory {
        pub data: Vec<u8>
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ReadRegister {
        pub value: u64
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct NextInstruction {
        pub addr: u64
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct SingleStep {
        pub value: bool
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct States {
        pub states: Vec<State>
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct State {
        pub tid: u64,
        pub state: u32
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ResponseError {
        pub msg: String
    }

    pub fn read<T: super::DeserializeOwned, R: super::io::Read>(reader: &mut R)
        -> super::Result<T> {

        let mut de = super::Deserializer::new(reader);

        let response_type: Type = super::Deserialize::deserialize(&mut de)?;
        let request_type: super::request::Type = super::Deserialize::deserialize(&mut de)?;

        match response_type {
            Type::Valid => Ok(super::Deserialize::deserialize(&mut de)?),
            Type::Error => {
                let err: ResponseError = super::Deserialize::deserialize(&mut de)?;
                let msg = format!("type {:?}: {}", request_type, err.msg);
                Err(super::ErrorKind::Request(msg).into())
            }
        }
    }

    pub fn read_no_data<R: super::io::Read>(reader: &mut R) -> super::Result<()> {
        let mut de = super::Deserializer::new(reader);

        let response_type: Type = super::Deserialize::deserialize(&mut de)?;
        let request_type: super::request::Type = super::Deserialize::deserialize(&mut de)?;

        match response_type {
            Type::Valid => Ok(()),
            Type::Error => {
                let err: ResponseError = super::Deserialize::deserialize(&mut de)?;
                let msg = format!("type {:?}: {}", request_type, err.msg);
                Err(super::ErrorKind::Request(msg).into())
            }
        }
    }
}

pub mod event {

    enum_number!(Type {
        Unknown = 0,
        Error = 1,
        Signal = 2,
        Breakpoint = 3,
        ThreadCreate = 4,
        ThreadDeath = 5,
        ProcessExit = 6,
        ProcessFork = 7,
        ProcessExec = 8,
        SingleStep = 9,
        ProcessCleanup = 10,
    });

    #[derive(Deserialize,Serialize,Debug)]
    pub struct EventError {
        pub msg: String
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct Signal {
        pub addr: u64,
        pub sig: u32
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct Breakpoint {
        pub addr: u64
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ThreadCreate {
        pub tid: u64
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ProcessExit {
        pub code: i32
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ProcessFork {
        pub pid: u32
    }

    #[derive(Deserialize,Serialize,Debug)]
    pub struct ProcessExec {
        pub path: String,
        pub argv: Vec<String>,
        pub envp: Vec<String>
    }

    #[derive(Debug, PartialEq)]
    pub enum EventData {
        Error{ msg: String },
        Signal{ addr: u64, sig: u32 },
        Breakpoint{ addr: u64 },
        ThreadCreate{ tid: u64 },
        ThreadDeath,
        ProcessExit{ code: i32 },
        ProcessFork{ pid: u32 },
        ProcessExec{ path: String, argv: Vec<String>, envp: Vec<String> },
        SingleStep,
        ProcessCleanup
    }

    #[derive(Debug)]
    pub struct EventMessage {
        pub tid: u64,
        pub data: EventData
    }
}

#[derive(Debug)]
pub enum EventReadError {
    Eof,
    Udi(Error)
}

impl From<serde_cbor::Error> for EventReadError {
    fn from(err: serde_cbor::Error) -> EventReadError {
        match err {
            serde_cbor::Error::Io(ioe) => {
                match ioe.kind() {
                    io::ErrorKind::UnexpectedEof => EventReadError::Eof,
                    _ => EventReadError::Udi(::std::convert::From::from(ioe))
                }
            },
            _ => EventReadError::Udi(::std::convert::From::from(err))
        }
    }
}

pub fn read_event<R: io::Read>(reader: R)
    -> ::std::result::Result<event::EventMessage, EventReadError> {

    let mut de = Deserializer::new(reader);

    let event_type: event::Type = Deserialize::deserialize(&mut de)?;
    let tid: u64 = Deserialize::deserialize(&mut de)?;

    let data = deserialize_event_data(&mut de, &event_type)?;

    Ok(event::EventMessage{ tid: tid, data: data })
}

fn deserialize_event_data<R: io::Read>(de: &mut Deserializer<R>, event_type: &event::Type)
    -> ::std::result::Result<event::EventData, EventReadError> {

    let event_data = match *event_type {
        event::Type::Unknown => {
            let msg = "Unknown event reported".to_owned();
            return Err(EventReadError::Udi(ErrorKind::Library(msg).into()));
        },
        event::Type::Error => {
            let error_data: event::EventError = Deserialize::deserialize(de)?;
            event::EventData::Error{ msg: error_data.msg }
        },
        event::Type::Signal => {
            let signal_data: event::Signal = Deserialize::deserialize(de)?;
            event::EventData::Signal{ addr: signal_data.addr, sig: signal_data.sig }
        },
        event::Type::Breakpoint => {
            let brkpt_data: event::Breakpoint = Deserialize::deserialize(de)?;
            event::EventData::Breakpoint{ addr: brkpt_data.addr }
        },
        event::Type::ThreadCreate => {
            let thr_create_data: event::ThreadCreate = Deserialize::deserialize(de)?;
            event::EventData::ThreadCreate{ tid: thr_create_data.tid }
        },
        event::Type::ThreadDeath => {
            event::EventData::ThreadDeath
        },
        event::Type::ProcessExit => {
            let exit_data: event::ProcessExit = Deserialize::deserialize(de)?;
            event::EventData::ProcessExit{ code: exit_data.code }
        },
        event::Type::ProcessFork => {
            let fork_data: event::ProcessFork = Deserialize::deserialize(de)?;
            event::EventData::ProcessFork{ pid: fork_data.pid }
        },
        event::Type::ProcessExec => {
            let exec_data: event::ProcessExec = Deserialize::deserialize(de)?;
            event::EventData::ProcessExec{
                path: exec_data.path,
                argv: exec_data.argv,
                envp: exec_data.envp
            }
        },
        event::Type::SingleStep => {
            event::EventData::SingleStep
        },
        event::Type::ProcessCleanup => {
            event::EventData::ProcessCleanup
        }
    };

    Ok(event_data)
}

enum_number!(Register {
    // X86 registers
    X86_MIN = 0,
    X86_GS = 1,
    X86_FS = 2,
    X86_ES = 3,
    X86_DS = 4,
    X86_EDI = 5,
    X86_ESI = 6,
    X86_EBP = 7,
    X86_ESP = 8,
    X86_EBX = 9,
    X86_EDX = 10,
    X86_ECX = 11,
    X86_EAX = 12,
    X86_CS = 13,
    X86_SS = 14,
    X86_EIP = 15,
    X86_FLAGS = 16,
    X86_ST0 = 17,
    X86_ST1 = 18,
    X86_ST2 = 19,
    X86_ST3 = 20,
    X86_ST4 = 21,
    X86_ST5 = 22,
    X86_ST6 = 23,
    X86_ST7 = 24,
    X86_MAX = 25,

    //X86_64 registers
    X86_64_MIN = 26,
    X86_64_R8 = 27,
    X86_64_R9 = 28,
    X86_64_R10 = 29,
    X86_64_R11 = 30,
    X86_64_R12 = 31,
    X86_64_R13 = 32,
    X86_64_R14 = 33,
    X86_64_R15 = 34,
    X86_64_RDI = 35,
    X86_64_RSI = 36,
    X86_64_RBP = 37,
    X86_64_RBX = 38,
    X86_64_RDX = 39,
    X86_64_RAX = 40,
    X86_64_RCX = 41,
    X86_64_RSP = 42,
    X86_64_RIP = 43,
    X86_64_CSGSFS = 44,
    X86_64_FLAGS = 45,
    X86_64_ST0 = 46,
    X86_64_ST1 = 47,
    X86_64_ST2 = 48,
    X86_64_ST3 = 49,
    X86_64_ST4 = 50,
    X86_64_ST5 = 51,
    X86_64_ST6 = 52,
    X86_64_ST7 = 53,
    X86_64_XMM0 = 54,
    X86_64_XMM1 = 55,
    X86_64_XMM2 = 56,
    X86_64_XMM3 = 57,
    X86_64_XMM4 = 58,
    X86_64_XMM5 = 59,
    X86_64_XMM6 = 60,
    X86_64_XMM7 = 61,
    X86_64_XMM8 = 62,
    X86_64_XMM9 = 63,
    X86_64_XMM10 = 64,
    X86_64_XMM11 = 65,
    X86_64_XMM12 = 66,
    X86_64_XMM13 = 67,
    X86_64_XMM14 = 68,
    X86_64_XMM15 = 69,
    X86_64_MAX = 70,
});
