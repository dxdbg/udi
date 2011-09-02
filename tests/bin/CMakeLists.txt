file(GLOB tests_globbed "*.c")
foreach(test_globbed ${tests_globbed})
    get_filename_component(test ${tests_globbed} NAME)
    set(tests ${test} ${tests})
endforeach()

foreach(test ${tests})
    STRING(REGEX REPLACE ".c" "" test_base_name ${test})
    add_executable(${test_base_name} ${test})

    # for now, link with libudirt
    target_link_libraries(${test_base_name} udirt)
endforeach()
