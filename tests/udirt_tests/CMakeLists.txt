include_directories("${PROJECT_SOURCE_DIR}/libudirt")
include_directories("${PROJECT_SOURCE_DIR}/libudicommon")
include_directories("${PROJECT_SOURCE_DIR}/tests/libuditest")

file(GLOB tests_globbed "*.cxx")
foreach(test_globbed ${tests_globbed})
    get_filename_component(test ${test_globbed} NAME)
    set(tests ${test} ${tests})
endforeach()

foreach(test ${tests})
    STRING(REGEX REPLACE ".cxx" "" test_base_name ${test})
    add_executable(${test_base_name} ${test})
    target_link_libraries(${test_base_name} udirt uditest)
endforeach()

add_executable(all_udirt_tests ${tests})
target_link_libraries(all_udirt_tests udirt uditest)
