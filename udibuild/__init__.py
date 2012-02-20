#
# Utility functionality used by the build
#
import platform
import sys
import os
import SCons.Errors
import subprocess
import re

def IsUnix():
    if platform.system() == 'Linux' or\
       platform.system() == 'FreeBSD':
        return True
    else:
        return False

def IsLittleEndian():
    if sys.byteorder == 'little':
        return True

def IsLinux():
    if platform.system() == 'Linux':
        return True
    else:
        return False

def IsX86():
    if platform.machine() == 'i386' or\
       platform.machine() == 'i686' or\
       platform.machine() == 'AMD64' or\
       platform.machine() == 'x86_64':
           return True
    else:
        return False

def IsExecutable(path):
    return os.path.exists(path) and os.access(path, os.X_OK)


def LocateExecutable(program):
    if IsExecutable(program):
        return program

    for path in os.environ["PATH"].split(os.pathsep):
        exe_path = os.path.join(path, program)
        if IsExecutable(exe_path):
            return exe_path

    return None


def GetFileExtension(filename):
    return os.path.splitext(filename)[1]

def GetFilename(filename):
    return os.path.splitext(filename)[0]

# extracting symbols from binaries
readelf = LocateExecutable("readelf")

nm = LocateExecutable("nm")

objdump = LocateExecutable("objdump")

def GetSymbolAddressReadelf(program, binary, symbol):
    proc = subprocess.Popen([program, '-s', binary], shell=False, stdout=subprocess.PIPE)
    lines = proc.communicate()[0].splitlines()

    for line in lines:
        if re.match('^\s+$', line) is not None:
            continue

        fields = re.split('\s+', line)

        if len(fields) == 9 and re.match('^\d+:', fields[1]) is not None:
            if fields[8] == symbol:
                return fields[2]

    symErrStr = 'Failed to locate symbol ' + symbol + ' in binary ' + binary

    raise SCons.Errors.BuildError(errstr=symErrStr)

def GetSymbolAddressNm(program, binary, symbol):
    raise SCons.Errors.BuildError(errstr='Unsupported symbol parser')

def GetSymbolAddressObjdump(program, binary, symbol):
    return SCons.Errors.BuildError(errstr='Unsupported symbol parser')

def GetSymbolAddress(binary, symbol):
    if readelf is not None:
        return GetSymbolAddressReadelf(readelf, binary, symbol)
    elif nm is not None:
        return GetSymbolAddressNm(nm, binary, symbol)
    elif objdump is not None:
        return GetSymbolAddressObjdump(objdump, binary, symbol)
    else:
        raise SCons.Errors.BuildError(errstr='Failed to locate symbol program')
