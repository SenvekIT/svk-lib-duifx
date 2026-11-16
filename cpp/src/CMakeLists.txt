# Every header in include/duifx and every source here belongs to the library.
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../include" DUIFX_INCLUDE_DIR)
file(GLOB DUIFX_HEADERS CONFIGURE_DEPENDS "${DUIFX_INCLUDE_DIR}/duifx/*.h")
file(GLOB DUIFX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# The Qt Designer forms of the Python package, compiled for C++.
qt_wrap_ui(DUIFX_FORMS
    "${DUIFX_SHARED_DIR}/gui/ui/main_window.ui"
    "${DUIFX_SHARED_DIR}/gui/ui/info_dialog.ui"
    "${DUIFX_SHARED_DIR}/gui/ui/settings_dialog.ui")

# The icons (resource.qrc, ":/icons/icon/<name>.svg" as in Python) and the
# stylesheet (":/duifx/modern_light.qss"). The resource initialiser is named
# after the .qrc file, and "resource" would clash with an application's own
# resource.qrc: so the shared file is rewritten here as duifx_icons.qrc, with
# absolute paths and the original names as aliases. See initResources().
function(duifx_relocated_qrc source target)
    get_filename_component(source_dir "${source}" DIRECTORY)
    file(READ "${source}" qrc)
    string(REGEX REPLACE "<file>([^<]+)</file>" "<file alias=\"\\1\">${source_dir}/\\1</file>" qrc "${qrc}")
    file(CONFIGURE OUTPUT "${target}" CONTENT "${qrc}" @ONLY)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
endfunction()
duifx_relocated_qrc("${DUIFX_SHARED_DIR}/gui/resources/resource.qrc" "${CMAKE_CURRENT_BINARY_DIR}/duifx_icons.qrc")
file(REAL_PATH "${DUIFX_SHARED_DIR}/gui/resources/modern_light.qss" DUIFX_QSS)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/duifx_styles.qrc" @ONLY CONTENT
"<RCC>\n  <qresource prefix=\"duifx\">\n    <file alias=\"modern_light.qss\">${DUIFX_QSS}</file>\n  </qresource>\n</RCC>\n")
qt_add_resources(DUIFX_RESOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/duifx_icons.qrc"
    "${CMAKE_CURRENT_BINARY_DIR}/duifx_styles.qrc")

add_library(duifx STATIC ${DUIFX_HEADERS} ${DUIFX_SOURCES} ${DUIFX_FORMS} ${DUIFX_RESOURCES})
add_library(duifx::duifx ALIAS duifx)

set_target_properties(duifx PROPERTIES AUTOMOC ON)
target_compile_features(duifx PUBLIC cxx_std_20)
target_include_directories(duifx
    PUBLIC "${DUIFX_INCLUDE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(duifx PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg)
target_compile_definitions(duifx PRIVATE DUIFX_VERSION_STRING="${PROJECT_VERSION}")

if(WIN32)
    # SetCurrentProcessExplicitAppUserModelID (taskbar grouping).
    target_link_libraries(duifx PRIVATE shell32)
endif()

if(MSVC)
    target_compile_options(duifx PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(duifx PRIVATE -Wall -Wextra -Wpedantic)
endif()
