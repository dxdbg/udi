# UDI Library - C Bindings #

This library provides a C ABI wrapper around the main libudi library.

## Platforms ##

Same as libudi.

## Building ##

The library is implemented in unsafe, stable Rust (1.22). The cargo build will produce a
libudi shared library that exports a C ABI. This shared library can be used in a C/C++
application directly or via a FFI for any other language.
