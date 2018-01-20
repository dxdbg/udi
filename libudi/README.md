# UDI Library #

This library implements the debugger side of the UDI protocol by sending requests,
receiving responses and events using a pseudo-filesystem for transport.

## Platforms ##

- linux/x86_64
- OS X/x86_64
- Windows/x86_64 (planned)

## Usage ##

This library has not yet been published to crates.io, but is planned.

## Building ##

This library can be built using stable Rust (1.22).

## Testing ##

`cargo test` will pull down a release from [native-file-tests](https://github.com/dxdbg/native-file-tests) and
launch binaries from that distribution as debuggees.

