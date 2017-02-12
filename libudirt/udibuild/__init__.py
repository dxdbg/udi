#
# Utility functionality used by the build
#
import platform
import sys
import os

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
