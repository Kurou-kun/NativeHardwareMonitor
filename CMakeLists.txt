cmake_minimum_required(VERSION 3.20)

project(NativeHardwareMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output DLL into Release/Debug folders like Visual Studio
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

# Collect source files
file(GLOB_RECURSE NHM_SOURCES
    source/*.cpp
)

file(GLOB_RECURSE NHM_HEADERS
    source/*.h
)

add_library(NativeHardwareMonitor SHARED
    ${NHM_SOURCES}
    ${NHM_HEADERS}
)

# Rainmeter plugin define
target_compile_definitions(NativeHardwareMonitor PRIVATE
    UNICODE
    _UNICODE
)

# Include directories
target_include_directories(NativeHardwareMonitor PRIVATE
    source
    external
)

# Windows libraries
target_link_libraries(NativeHardwareMonitor PRIVATE
    advapi32
    psapi
    iphlpapi
)

# Remove lib prefix
set_target_properties(NativeHardwareMonitor PROPERTIES
    PREFIX ""
)

# Set DLL output name
set_target_properties(NativeHardwareMonitor PROPERTIES
    OUTPUT_NAME "NativeHardwareMonitor"
)