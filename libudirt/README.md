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

### Installing the dependencies on Ubuntu 22.04

#### libcbor

```
sudo snap install --classic cmake
mkdir libcbor && cd libcbor
curl -OL https://github.com/PJK/libcbor/archive/refs/tags/v0.10.2.zip
unzip v0.10.2.zip
cd libcbor-0.10.2
# Build a static lib (.a) with relocatable code suitable for linking into a shared library
export CFLAGS=-fPIC
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF .
make
sudo make install
```

#### libudis86

```
sudo apt install libtool autoconf
git clone --branch d95a9d1 --depth=1 https://github.com/BlueArc0/udis86.git
cd udis86
./autogen.sh
export CFLAGS=-fPIC
./configure --prefix=/usr/local --with-python=python3 --disable-shared --enable-static
make
sudo make install
```

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

Paths to the dependencies can be set via `scons configure`. If you followed the above directions on Ubuntu 22.04,
the following command will point SCons to the installed dependencies.
