# 
# Top level SConscript file for udi projects
#

import sys
import os

# import utility functionality
sys.path.append(os.path.abspath('udibuild'))
import udibuild

topenv = Environment()

Export('topenv')

if topenv['CC'] == 'gcc':
    # C compiler flags
    topenv.Append(CFLAGS = "-Wall -fPIC -std=gnu99 -g")

    # C++ compiler flags
    topenv.Append(CXXFLAGS = "-Wall -g -fPIC")
else:
    print 'Unknown compiler'
    quit()

# platform and architecture specific defines
if udibuild.IsUnix():
    topenv.Append(CPPDEFINES=['UNIX'])
else:
    topenv.Append(CPPDEFINES=['WINDOWS'])

if udibuild.IsLittleEndian():
    topenv.Append(CPPDEFINES=['LITTLE_ENDIAN'])
else:
    topenv.Append(CPPDEFINES=['BIG_ENDIAN'])

if udibuild.IsLinux():
    topenv.Append(CPPDEFINES=['LINUX'])

# subdirectories
topenv.SConscript('#/libudi/SConscript', variant_dir='#/build/libudi', duplicate=False)
topenv.SConscript('#/libudicommon/SConscript', variant_dir='#/build/libudicommon', duplicate=False)
topenv.SConscript('#/libudirt/SConscript', variant_dir='#/build/libudirt', duplicate=False)
topenv.SConscript('#/tests/bin/SConscript', variant_dir='#/build/tests/bin', duplicate=False)
topenv.SConscript('#/tests/libuditest/SConscript', variant_dir='#/build/tests/libuditest', duplicate=False)
topenv.SConscript('#/tests/udirt_tests/SConscript', variant_dir='#/build/tests/udirt_tests', duplicate=False)
topenv.SConscript('#/tests/udi_tests/SConscript', variant_dir='#/build/tests/udi_tests', duplicate=False)

# default target
topenv.Default('libudi', 'libudirt', 'tests/udirt_tests', 'tests/udi_tests')
