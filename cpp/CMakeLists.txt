cmake_minimum_required(VERSION 3.21)

# DUIFx for C++/Qt Widgets. The Python package (../duifx) is the reference:
# both share the stylesheet, the icons and the Qt Designer forms, which stay in
# ../duifx/gui and exist only once.
set(DUIFX_SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/../duifx")

# One version for both languages: duifx/version.py.
file(STRINGS "${DUIFX_SHARED_DIR}/version.py" _duifx_version_line REGEX "^__version__")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" DUIFX_VERSION "${_duifx_version_line}")

project(duifx
    VERSION ${DUIFX_VERSION}
    DESCRIPTION "DUIFx - Senvek user interface framework for Qt Widgets"
    LANGUAGES CXX)

# Only the top-level build sets the language level; a parent project keeps its own.
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

option(DUIFX_BUILD_TESTS "Build the DUIFx unit tests" ${PROJECT_IS_TOP_LEVEL})
option(DUIFX_BUILD_EXAMPLES "Build the DUIFx examples" ${PROJECT_IS_TOP_LEVEL})

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Svg)

add_subdirectory(src)

if(DUIFX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(DUIFX_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
