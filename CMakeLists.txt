cmake_minimum_required(VERSION 3.16)
set(ProjectMainName VorzeControlServer)
project(${ProjectMainName})

set(CMAKE_CXX_STANDARD 17)
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DBOOST_ALL_NO_LIB)

    add_definitions(-DMSVC)
    if (COMMAND _WIN32_WINNT)
    else ()
        # if _WIN32_WINNT not set. set the _WIN32_WINNT to 0x0A00 (Windows 10)
        # you can add " /D_WIN32_WINNT=0x0601" in CMAKE_CXX_FLAGS to set it to 0x0601 (win7)
        add_definitions(-D_WIN32_WINNT=0x0A00)
    endif ()
    add_compile_options(/bigobj)

    add_definitions(/W4)
else ()
    add_definitions(-Wall -Wextra)
endif ()

if (${ProjectMainName}_NOT_USE_EmbedWebServer)
    add_definitions(-DDISABLE_EmbedWebServer)
    message(STATUS "NOT_USE_EmbedWebServer !!!")
endif ()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(NOT_USE_BOOST_THEAD "Config NOT_USE_BOOST_THEAD")

if (NOT_USE_BOOST_THEAD)
    set(BOOST_THEAD_MODULE "")
    message(STATUS "NOT_USE_BOOST_THEAD !!!")
else ()
    set(BOOST_THEAD_MODULE "thread")
    add_definitions(-DUSE_BOOST_THEAD)
endif ()

option(USE_DYNAMIC_BOOST "Config USE_DYNAMIC_BOOST")

if (USE_DYNAMIC_BOOST)
    message(STATUS "USE DYNAMIC BOOST !!!")
    add_definitions(-DBOOST_ALL_DYN_LINK=1)
else ()
    message(STATUS "USE STATIC BOOST !!!")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif ()
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.70.0 REQUIRED COMPONENTS system filesystem program_options ${BOOST_THEAD_MODULE} REQUIRED)


message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

if (MSVC)
else ()
    #    if (${Boost_VERSION} VERSION_LESS_EQUAL 1.72.0)
    #        # https://github.com/boostorg/asio/issues/312
    #        # https://github.com/microsoft/vcpkg/issues/9731
    #        # https://github.com/boostorg/beast/issues/1884
    #        add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)
    #        message(STATUS "Boost_VERSION <= 1.72")
    #    endif ()
endif ()

find_package(OpenSSL 1.1.0 COMPONENTS SSL Crypto REQUIRED)

# custom 3th lib
# https://stackoverflow.com/questions/20746936/what-use-is-find-package-if-you-need-to-specify-cmake-module-path-anyway
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# find wmi
set(Wmi_ROOT src/3thlib/wmi)
find_package(Wmi REQUIRED)

message(STATUS "WMI_INCLUDE_DIRS: ${WMI_INCLUDE_DIRS}")
message(STATUS "WMI_LIBRARIES: ${WMI_LIBRARIES}")

# find wmi
set(PromiseCpp_ROOT src/3thlib/promise-cpp)
find_package(PromiseCpp REQUIRED)
message(STATUS "PromiseCpp_INCLUDE_DIRS: ${PromiseCpp_INCLUDE_DIRS}")
message(STATUS "PromiseCpp_FOUND: ${PromiseCpp_FOUND}")
if (PromiseCpp_FOUND)
    message(STATUS "PromiseCpp_FOUND")
    add_definitions(-DPromiseCpp_FOUND=1)
    if (NOT NOT_USE_BOOST_THEAD)
        message(STATUS "PM_MULTITHREAD Mode")
        add_definitions(-DPM_MULTITHREAD=1)
    endif ()
endif ()

# https://github.com/jbeder/yaml-cpp
find_package(yaml-cpp REQUIRED)
message(STATUS "YAML_CPP_LIBRARIES: ${YAML_CPP_LIBRARIES}")
message(STATUS "YAML_CPP_INCLUDE_DIR: ${YAML_CPP_INCLUDE_DIR}")
message(STATUS "yaml-cpp_FOUND: ${yaml-cpp_FOUND}")
if (YAML_CPP_LIBRARIES)
    message(STATUS "yaml-cpp_FOUND")
endif ()


add_executable(${ProjectMainName}
        src/main.cpp
        src/ConfigLoader.cpp
        src/ConfigLoader.h
        src/error_info.cpp
        src/error_info.h
        src/WebControlServer.cpp
        src/WebControlServer.h
        src/EmbedWebServer.cpp
        src/EmbedWebServer.h
        src/SerialPortControlServer.cpp
        src/SerialPortControlServer.h
        src/ActionModeManager.cpp
        src/ActionModeManager.h
        src/AsyncDelay.cpp
        src/AsyncDelay.h
        src/SerialPortFinder.cpp
        src/SerialPortFinder.h)

# include wmi
include_directories(${WMI_INCLUDE_DIRS})
target_link_libraries(${ProjectMainName} ${WMI_LIBRARIES})

# include PromiseCpp
include_directories(${PromiseCpp_INCLUDE_DIRS})

# include yaml-cpp
include_directories(${YAML_CPP_INCLUDE_DIR})
target_link_libraries(${ProjectMainName} ${YAML_CPP_LIBRARIES})

target_link_libraries(${ProjectMainName} ${CMAKE_THREAD_LIBS_INIT})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${ProjectMainName} ${Boost_LIBRARIES})
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${ProjectMainName} ${OPENSSL_LIBRARIES})
if (OPENSSL_VERSION VERSION_GREATER_EQUAL 1.1.1)
    option(ENABLE_SSL_KEYLOG "Build with SSL KeyLog support" ON)
    if (ENABLE_SSL_KEYLOG)
        add_definitions(-DENABLE_SSL_KEYLOG)
    endif ()

    option(ENABLE_TLS13_CIPHERSUITES "Build with TLS1.3 ciphersuites support" ON)
    if (ENABLE_TLS13_CIPHERSUITES)
        add_definitions(-DENABLE_TLS13_CIPHERSUITES)
    endif ()
endif ()

if (WIN32)
    target_link_libraries(${ProjectMainName} wsock32 ws2_32 crypt32)
else ()
endif ()

