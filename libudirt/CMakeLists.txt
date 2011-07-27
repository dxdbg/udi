set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

# platform and architecture independent files
set(udirt_SOURCES udirt.c)

if(IS_BIG_ENDIAN)
    set(udirt_SOURCES udirt-big.c ${udirt_SOURCES})
else(IS_BIG_ENDIAN)
    set(udirt_SOURCES udirt-little.c ${udirt_SOURCES})
endif(IS_BIG_ENDIAN)

if(UNIX)
    set(udirt_SOURCES udirt-posix.c ${udirt_SOURCES})
else(UNIX)
    # TODO
endif(UNIX)

add_library(libudirt ${udirt_SOURCES})
