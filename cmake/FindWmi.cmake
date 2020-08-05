cmake_minimum_required(VERSION 3.16)

message(STATUS "Wmi_ROOT: ${Wmi_ROOT}")

set(WMI_INCLUDE_DIRS
        ${Wmi_ROOT}/include
)

set(WMI_SOURCES
        ${Wmi_ROOT}/src/wmi.cpp
        ${Wmi_ROOT}/src/wmiresult.cpp)

add_library(wmi STATIC ${WMI_SOURCES})

target_link_libraries(wmi ole32 oleaut32 wbemuuid)

set(WMI_LIBRARIES
        wmi)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -c -O3 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64  -fipa-pure-const ")
endif ()
