cmake_minimum_required(VERSION 3.10.0...3.30.0)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(atomix VERSION 0.1.0 LANGUAGES CXX)
add_subdirectory(src)

set (ENV{CMAKE_BUILD_PARALLEL_LEVEL} 16)
target_compile_features(atomix PRIVATE cxx_std_23)
if (WIN32)
    target_compile_options(atomix PRIVATE /W4 /wd4267)
else()
    target_compile_options(atomix PRIVATE -std=c++23 -Wall -Wextra -pedantic)
endif()
