set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${PROJECT_SOURCE_DIR}/libudicommon")
include_directories("${PROJECT_SOURCE_DIR}/libudirt")

set(libudi_SOURCES libudi.c)

if(UNIX)
    add_definitions(-DUNIX)
    set(libudi_SOURCES ${libudi_SOURCES} libudi-posix.c)
else(UNIX)
    # TODO
endif(UNIX)

add_library(udi ${libudi_SOURCES})
target_link_libraries(udi udicommon)
