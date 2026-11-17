add_executable(duifx_demo WIN32 demo/main.cpp)
target_link_libraries(duifx_demo PRIVATE duifx::duifx)

# Copy the Qt DLLs and plugins next to the demo so it starts with a double
# click, without Qt's bin folder on PATH.
if(WIN32 AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET duifx_demo POST_BUILD
        COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime --no-system-d3d-compiler
                --no-opengl-sw --verbose 0 "$<TARGET_FILE:duifx_demo>"
        COMMENT "windeployqt duifx_demo"
        VERBATIM)
endif()
