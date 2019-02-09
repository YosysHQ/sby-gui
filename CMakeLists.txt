# TODO: sensible minimum CMake version
cmake_minimum_required(VERSION 3.3)
project(sby-gui)

set(CMAKE_CXX_STANDARD 11)

option(BUILD_TESTS "Build Test" ON)

if (MSVC)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG /W4")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W4")
else()
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -fPIC -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -fPIC -O3 -g")
endif()

if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
     set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
endif()

# List of Boost libraries to include
set(boost_libs filesystem)
find_package(Boost REQUIRED COMPONENTS ${boost_libs})
# Find the Qt5 libraries
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

add_subdirectory(3rdparty/scintilla ${CMAKE_CURRENT_BINARY_DIR}/generated/3rdparty/ScintillaEdit)
add_subdirectory(src ${CMAKE_CURRENT_BINARY_DIR}/generated/src)

if (BUILD_TESTS)
    add_subdirectory(3rdparty/googletest/googletest ${CMAKE_CURRENT_BINARY_DIR}/generated/3rdparty/googletest EXCLUDE_FROM_ALL)
    enable_testing()
    aux_source_directory(tests/ ALL_TEST_FILES)  
    aux_source_directory(src/common COMMON_SOURCE_FILES)
    add_executable(sbytest ${ALL_TEST_FILES} ${COMMON_SOURCE_FILES}) 
    target_include_directories(sbytest PRIVATE src/common)
    target_link_libraries(sbytest PRIVATE gtest_main)
    add_test(sbytest ${CMAKE_CURRENT_BINARY_DIR}/sbytest)
endif()

set(EXECUTABLE_OUTPUT_PATH .)

file(GLOB_RECURSE CLANGFORMAT_FILES *.cc *.h)
string(REGEX REPLACE "[^;]*/3rdparty[^;]*" "" CLANGFORMAT_FILES "${CLANGFORMAT_FILES}")
string(REGEX REPLACE "[^;]*/generated[^;]*" "" CLANGFORMAT_FILES "${CLANGFORMAT_FILES}")

add_custom_target(
    clangformat
    COMMAND clang-format
    -style=file
    -i
    ${CLANGFORMAT_FILES}
)
