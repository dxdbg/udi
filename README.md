Userland Debugger Interface
===========================

[![Build Status](https://travis-ci.org/dxdbg/udi.svg?branch=master)](https://travis-ci.org/dxdbg/udi)

The Userland Debugger Interface (UDI) intends to provide a full-featured
debugger interface for native applications, implemented completely in userland
without utilizing debugging APIs provided by the OS/kernel.

For a discussion of the motivation for this project, see the
[design document](docs/DESIGN.md).

## Protocol ##

The foundation of UDI is a [CBOR](http://cbor.io/)-based protocol that defines
the communication between a debuggee and a debugger.

For more information about the protocol, see the [protocol document](docs/PROTOCOL.md).

## Libraries ##

This project provides the following libraries:

### libudi ###

The [libudi](libudi/README.md) library, written in Rust, provides an API
for controlling a debuggee. This library is intended to be a fundamental part of
a debugger and uses a pseudo-filesystem as the transport mechanism for the UDI
protocol.

### libudirt ###

The [libudirt](libudirt/README.md) library, written in C, implements the
debuggee side of the UDI protocol via a pseudo-filesystem, with no cooperation
or modification to the native application being debugged.

### libudi-c ###

The [libudi-c](libudi-c/README.md) library, written in Rust, is a wrapper
around libudi that provides a C ABI API to libudi.

### libudi-java ###

The [libudi-java](libudi-java/README.md) library, written in Java, is a wrapper
around libudi-c that provides a Java API to libudi.

## License ##

UDI is licensed under the [MPL 2](https://www.mozilla.org/MPL/2.0/).

## Contributors ##

[The AUTHORS file](docs/AUTHORS) provides a list of contributors to the UDI
project.
