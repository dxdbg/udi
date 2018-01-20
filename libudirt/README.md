# UDI Runtime Library #

This library implements the debuggee side of the UDI protocol by receiving requests,
sending responses and events using a pseudo-filesystem for transport.

## Platforms ##

- linux/x86_64
- OS X/x86_64
- Windows/x86_64 (planned)

## Dependencies ##

- [libcbor](https://github.com/PJK/libcbor)
- [libudis86](https://github.com/vmt/udis86)

## Building ##

This library uses [SCons](http://www.scons.org/) as its build system.

If the dependencies are installed in standard locations, the library can be built with:

```
scons
```

If the dependencies are installed in nonstandard locations, consult the online help for more info:

```
scons -h
```

The shared library artifact will be in `build/src` when the build is successful.
