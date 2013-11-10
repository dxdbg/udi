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

def GetSymbolInfoObjdump(program, binary, symbol):
    proc = subprocess.Popen([program, '-t', binary], shell=False, stdout=subprocess.PIPE)
    lines = proc.communicate()[0].splitlines()

    for line in lines:
        fields = re.split('\s+', line)
        if len(fields) == 6:
            if fields[5] == symbol:
                return (fields[0], fields[4])

    symErrStr = 'Failed to locate symbol ' + symbol + ' in binary ' + binary + ' with program ' + program

    raise SCons.Errors.BuildError(errstr=symErrStr)

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

def GetSymbolLengthObjdump(program, binary, symbol):
    ( symaddr, symlength ) = GetSymbolInfoObjdump(program, binary, symbol)

    return int(symlength, 16)

def GetSymbolLength(binary, symbol):
    if objdump is not None:
        return GetSymbolLengthObjdump(objdump, binary, symbol)

def GetInstructionLengthObjdump(program, binary, symbol):
    # First get the length of the function
    (symaddr, symlength) = GetSymbolInfoObjdump(program, binary, symbol)

    addr = int(symaddr, 16)
    length = int(symlength, 16)

    stopaddr = addr + length

    arguments = [program, 
            '-d',
            '--start-address={0}'.format(hex(addr)),
            '--stop-address={0}'.format(hex(stopaddr)),
            binary]

    proc = subprocess.Popen(arguments, shell=False, stdout=subprocess.PIPE)
    lines = proc.communicate()[0].splitlines()

    nextline = False
    for line in lines:
        if not nextline:
            if re.match('^\S+ <\S+>:$', line) is not None:
                nextline = True
        else:
            fields = re.split('\s+', line)
            if len(fields) >= 4:
                start = 2
                while start < len(fields):
                    if re.match('^[0-9a-e]+$', fields[start]) is not None:
                        start += 1
                    else:
                        return start - 2

    instErrStr = 'Failed to determine instruction length for ' + symbol +\
        ' in binary ' + binary

    raise SCons.Errors.BuildError(errstr=instErrStr)

def GetInstructionLength(binary, symbol):
    '''
    Gets the length of the instruction at the specified symbol
    '''
    if objdump is not None:
        return GetInstructionLengthObjdump(objdump, binary, symbol)

    raise SCons.Errors.BuildError(errstr='Failed to locate disassembly program')
