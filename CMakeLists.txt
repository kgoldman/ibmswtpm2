cmake_minimum_required(VERSION 3.12)

project(ibmswtpm2 VERSION 1563 LANGUAGES C)

include (GNUInstallDirs)
include(TestBigEndian)

if(APPLE OR MSVC)
 include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
 conan_basic_setup()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenSSL REQUIRED)
if(UNIX)
    find_package(Threads REQUIRED)
endif()

add_subdirectory(src)
add_subdirectory(utils)