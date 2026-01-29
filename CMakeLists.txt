cmake_minimum_required(VERSION 3.10)
project(iirdsp C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration option for precision
option(IIRDSP_USE_FLOAT "Use float instead of double" OFF)

if(IIRDSP_USE_FLOAT)
    add_compile_definitions(IIRDSP_USE_FLOAT)
endif()

# Core library (C implementation)
add_library(iirdsp_core STATIC
    src/sos.c
    src/butter.c
    src/notch.c
)

target_include_directories(iirdsp_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iirdsp_core PUBLIC m)

# C++ wrapper (header-only, optional)
add_library(iirdsp INTERFACE)
target_include_directories(iirdsp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp>
    $<INSTALL_INTERFACE:include>
)

# Examples
if(NOT EMBEDDED_BUILD)
    add_executable(ecg_desktop examples/ecg_desktop.c)
    target_link_libraries(ecg_desktop PRIVATE iirdsp_core m)
    target_include_directories(ecg_desktop PRIVATE include)
endif()

# Tests
enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/impulse.cpp")
    add_executable(test_impulse tests/impulse.cpp)
    target_link_libraries(test_impulse PRIVATE iirdsp_core m)
    target_include_directories(test_impulse PRIVATE include cpp)
    add_test(NAME impulse COMMAND test_impulse)
endif()

# Installation
install(TARGETS iirdsp_core iirdsp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(DIRECTORY cpp/
    DESTINATION include
)
