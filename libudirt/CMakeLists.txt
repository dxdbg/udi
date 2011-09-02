set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${PROJECT_SOURCE_DIR}/libudicommon")

# platform and architecture independent files
set(udirt_SOURCES udirt.c)

if(UNIX)
    add_definitions(-DUNIX)
    set(udirt_SOURCES udirt-posix.c ${udirt_SOURCES})
else(UNIX)
    # TODO
endif(UNIX)

add_library(udirt ${udirt_SOURCES})

## TODO not all unices have a separate libdl
target_link_libraries(udirt udicommon dl)
