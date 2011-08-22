set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${PROJECT_SOURCE_DIR}/libudicommon")
include_directories("${PROJECT_SOURCE_DIR}/libudirt")

set(libudi_SOURCES libudi.c)

add_library(udi ${libudi_SOURCES})
