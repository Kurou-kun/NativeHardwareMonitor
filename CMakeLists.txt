cmake_minimum_required(VERSION 3.20)

project(NativeHardwareMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    source/*.cpp
)

add_library(NativeHardwareMonitor SHARED ${SOURCES})

target_include_directories(NativeHardwareMonitor PRIVATE
    source
    external/rainmeter
    external/nvml
    external/adlx
)

target_compile_definitions(NativeHardwareMonitor PRIVATE
    UNICODE
    _UNICODE
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