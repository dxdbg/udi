include_directories("${PROJECT_SOURCE_DIR}/libudi")
include_directories("${PROJECT_SOURCE_DIR}/tests/libuditest")
include_directories("${PROJECT_SOURCE_DIR}/libudirt")

add_definitions("-DMACRO_BINARY_DIR=\"${CMAKE_BINARY_DIR}/tests/bin\"")

file(GLOB tests_globbed "*.cxx")
foreach(test_globbed ${tests_globbed})
    get_filename_component(test ${test_globbed} NAME)
    set(tests ${test} ${tests})
endforeach()

foreach(test ${tests})
    STRING(REGEX REPLACE ".cxx" "" test_base_name ${test})
    add_executable(${test_base_name} ${test})
    target_link_libraries(${test_base_name} udi uditest)
endforeach()

add_executable(all_libudi_tests ${tests})
target_link_libraries(all_libudi_tests udi uditest)
