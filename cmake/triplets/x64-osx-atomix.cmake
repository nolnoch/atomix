# Static, release-only third-party libs for packaged macOS builds.
# Deployment target must match CMAKE_OSX_DEPLOYMENT_TARGET in the top-level CMakeLists.txt.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.4)
set(VCPKG_BUILD_TYPE release)
# CMake 4 (which vcpkg uses for port builds) refuses projects declaring cmake_minimum_required < 3.5;
# some ports at the manifest baseline may still do.
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_POLICY_VERSION_MINIMUM=3.5)
