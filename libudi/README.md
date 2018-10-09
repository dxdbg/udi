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

This library can be built using Rust 1.25+.

## Testing ##

The integration tests expect that a release from [native-file-tests](https://github.com/dxdbg/native-file-tests) has
been extracted as the directory `native-file-tests` in the cargo manifest directory. The tests will launch binaries
from this distribution as debuggees.

## Implementation Notes

### Runtime Library Loading

#### Debugger creates debuggee

##### linux

The runtime library path is specified in the LD\_PRELOAD environment variable
when creating the debuggee.

##### macos

The runtime library path is specified in the DYLD\_INSERT\_LIBRARIES
environment variable. DYLD\_FORCE\_FLAT\_NAMESPACE is also set to support
hooking functions provided by system runtime libraries (e.g., pthreads).

##### windows

When creating the debuggee, it is created in a suspended state, using the
CREATE\_SUSPENDED process creation flag. A remote thread is created in the
process, with an entry point of the `LoadLibrary` function and the argument to
the thread set to the path to the runtime library. The execution of this remote
thread then loads the runtime library into the debuggee. The debugger then
resumes the main thread which triggers initialization of the runtime
library via it's DllMain function.
