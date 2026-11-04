cmake_minimum_required(VERSION 3.30...4.2)

# Use vcpkg for third-party libraries when VCPKG_ROOT is set and no toolchain was given
# (presets pass CMAKE_TOOLCHAIN_FILE explicitly; this is only a convenience for bare `cmake -S . -B build`).
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain")
endif()

# Qt 6.12 requires macOS 14.4+. Must be set before project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4" CACHE STRING "Minimum macOS version")

project(atomix
    VERSION 0.2.0
    DESCRIPTION "Vulkan renderer for 3D waves and spherical harmonics"
    HOMEPAGE_URL "https://github.com/nolnoch/atomix"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows packages are a flat folder (atomix.exe + DLLs + data). Qt's deploy script reads this
# to find the exe, so it must be set before GNUInstallDirs runs (in src/CMakeLists.txt).
if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Executable install dir")
endif()

add_subdirectory(src)
