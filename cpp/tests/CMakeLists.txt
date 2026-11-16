# One test executable per tst_*.cpp.
file(GLOB DUIFX_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tst_*.cpp")

foreach(test_source IN LISTS DUIFX_TESTS)
    get_filename_component(test_name "${test_source}" NAME_WE)
    add_executable(${test_name} "${test_source}" TestSupport.h)
    set_target_properties(${test_name} PROPERTIES AUTOMOC ON)
    target_link_libraries(${test_name} PRIVATE duifx::duifx Qt6::Test)
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4 /permissive- /utf-8)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
    # Widgets without a display.
    set_tests_properties(${test_name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
