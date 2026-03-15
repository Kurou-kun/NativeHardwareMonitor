cmake_minimum_required(VERSION 3.20)

project(NativeHardwareMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(
    UNICODE
    _UNICODE
    NOMINMAX
)

# Collect project sources
file(GLOB_RECURSE SOURCES
    source/*.cpp
)

# Add ADLX helper implementation
set(EXTERNAL_SOURCES
    external/amd/adlx/ADLXHelper.cpp
)

add_library(NativeHardwareMonitor SHARED
    ${SOURCES}
    ${EXTERNAL_SOURCES}
)

target_include_directories(NativeHardwareMonitor PRIVATE
    source
    external/rainmeter
    external/nvidia
    external/amd/adlx
)

target_link_libraries(NativeHardwareMonitor PRIVATE
    advapi32
    psapi
    iphlpapi
)

set_target_properties(NativeHardwareMonitor PROPERTIES
    PREFIX ""
    OUTPUT_NAME "NativeHardwareMonitor"
)