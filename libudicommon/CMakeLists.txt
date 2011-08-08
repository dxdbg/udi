set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${PROJECT_SOURCE_DIR}/libudirt")

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

# platform and architecture independent files
set(udicommon_SOURCES udi-common.c)

if(IS_BIG_ENDIAN)
    set(udicommon_SOURCES udi-common-big.c ${udicommon_SOURCES})
else(IS_BIG_ENDIAN)
    set(udicommon_SOURCES udi-common-little.c ${udicommon_SOURCES})
endif(IS_BIG_ENDIAN)

if(UNIX)
    set(udicommon_SOURCES udi-common-posix.c ${udicommon_SOURCES})
else(UNIX)
    # TODO
endif(UNIX)

add_library(udicommon ${udicommon_SOURCES})
