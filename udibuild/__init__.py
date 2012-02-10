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

def GetFileExtension(filename):
    return os.path.splitext(filename)[1]

def GetFilename(filename):
    return os.path.splitext(filename)[0]
