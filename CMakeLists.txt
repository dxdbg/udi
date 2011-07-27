cmake_minimum_required(VERSION 2.8)

project(udi)

# Originally found here:
# http://cabledogs.org/abinader/2009/12/07/how-cmake-simplifies-the-build-process-part-1-basic-build-system/

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which
# stores the directories of additional CMake modules (eg
# MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
 
# The macro below forces the build directory to be different from source
# directory:
include(MacroOutOfSourceBuild)
 
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out of source build.")

# Subdirectories
add_subdirectory(libudi)
add_subdirectory(libudirt)
add_subdirectory(tests)
