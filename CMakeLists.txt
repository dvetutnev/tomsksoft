# For TomskSoft
# Dmitriy Vetutnev 2021

cmake_minimum_required(VERSION 3.7)
project(simple-server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

include(conan.cmake)

conan_cmake_autodetect(settings)
conan_cmake_install(
    PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}/conanfile.txt
    BUILD missing
    REMOTE conan-center
    SETTINGS ${settings})

find_package(sml REQUIRED)
find_package(libuv REQUIRED)
find_package(uvw REQUIRED)
find_package(GTest REQUIRED)

include_directories(src)

enable_testing()

add_subdirectory(test)
