# Userland Debugger Interface #

The Userland Debugger Interface (UDI) attempts to provide a full-featured
debugger interface completely in userland with little to no support from the
kernel. One of the major goals of UDI is to provide a portable debugger 
library that provides the base functionality necessary for cross-platform
debugging tools.

For a discussion of the motivation for this project, see the DESIGN document.

## Dependencies ##

The runtime library, libudirt, is statically linked with libudis86, a 
lightweight disassembler library for the x86/x86_64 instruction sets,
available at http://udis86.sourceforge.net/.

## Building ##

UDI should build on any modern Linux x86/x86_64 distro with the appropriate
tools installed:

* scons
* gcc/g++
* libudis86 (headers and library)

## License ##

UDI is licensed under the [MPL 2](https://www.mozilla.org/MPL/2.0/).
