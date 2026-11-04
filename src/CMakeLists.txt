#
# atomix executable, install layout, and per-platform bundling.
#
# Dependencies come from:
#   - Qt 6:                CMAKE_PREFIX_PATH (presets read it from $QT_ROOT_DIR)
#   - everything else:     vcpkg manifest (vcpkg.json), except oneDPL
#   - oneDPL (header-only): an installed copy if find_package finds one, otherwise downloaded here
#   - MoltenVK (macOS):    ATOMIX_MOLTENVK_DIR, or found in Homebrew / Vulkan SDK locations
#

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Concurrent Qml)
qt_standard_project_setup()   # AUTOMOC + GNUInstallDirs

find_package(TBB CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

# --- oneDPL ----------------------------------------------------------------
# Header-only; supplies parallel std::execution algorithms where the standard library
# lacks them (Apple libc++). We only need the headers, so the release archive is
# downloaded without running oneDPL's own CMake (which would add its install rules).
find_package(oneDPL CONFIG QUIET)
if(NOT TARGET oneDPL)
    include(FetchContent)
    FetchContent_Declare(onedpl
        URL https://github.com/uxlfoundation/oneDPL/archive/refs/tags/oneDPL-release-2022.14.0.tar.gz
        URL_HASH SHA256=1e40549d300265aac5459e6e52b5aa5ae9073586067c4a4a38a21c3511ca0373
        SOURCE_SUBDIR _headers_only_   # nonexistent: download, don't add_subdirectory
    )
    FetchContent_MakeAvailable(onedpl)
    add_library(oneDPL INTERFACE)
    target_include_directories(oneDPL SYSTEM INTERFACE "${onedpl_SOURCE_DIR}/include")
    target_compile_definitions(oneDPL INTERFACE ONEDPL_USE_DPCPP_BACKEND=0)
    target_link_libraries(oneDPL INTERFACE TBB::tbb)
    if(MSVC)
        target_compile_options(oneDPL INTERFACE /Zc:__cplusplus /EHsc)
    endif()
    message(STATUS "oneDPL: using downloaded headers at ${onedpl_SOURCE_DIR}")
else()
    message(STATUS "oneDPL: using installed package at ${oneDPL_DIR}")
endif()

# --- Executable ------------------------------------------------------------
set(ATOMIX_SOURCES
    main.cpp quaternion.cpp special.cpp shaderobj.cpp mainwindow.cpp filehandler.cpp
    manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programVK.cpp vkwindow.cpp
)
set(ATOMIX_ICNS "${PROJECT_SOURCE_DIR}/res/icons/atomix.icns")

if(APPLE)
    set_source_files_properties(${ATOMIX_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    qt_add_executable(atomix MACOSX_BUNDLE ${ATOMIX_SOURCES} ${ATOMIX_ICNS})
elseif(WIN32)
    qt_add_executable(atomix WIN32 ${ATOMIX_SOURCES} "${PROJECT_SOURCE_DIR}/packaging/windows/atomix.rc")
else()
    qt_add_executable(atomix ${ATOMIX_SOURCES})
endif()

target_compile_definitions(atomix PRIVATE
    ATOMIX_VERSION="${PROJECT_VERSION}"
    # Last-resort location for configs/ and shaders/ when running straight from a build tree.
    ATOMIX_DEV_DIR="${PROJECT_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(atomix PRIVATE /W4 /wd4267)
else()
    target_compile_options(atomix PRIVATE -Wall -Wextra -pedantic)
endif()

target_link_libraries(atomix PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent Qt6::Qml
    glm::glm
    glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV
    SPIRV-Tools-static SPIRV-Tools-opt
    spirv-cross-core spirv-cross-reflect
    oneDPL TBB::tbb TBB::tbbmalloc
    Vulkan::Vulkan
)

if(APPLE)
    set_target_properties(atomix PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.nolnoch.atomix"
        MACOSX_BUNDLE_BUNDLE_NAME "atomix"
        MACOSX_BUNDLE_ICON_FILE "atomix.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright 2023-2026 Wade Burch (GPLv3)"
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

# --- Install layout --------------------------------------------------------
#   macOS:   <prefix>/atomix.app (data in Contents/Resources, dylibs in Contents/Frameworks)
#   Windows: <prefix>/atomix.exe with data and Qt DLLs alongside (flat, zip-friendly)
#   Linux:   <prefix>/bin/atomix, <prefix>/share/atomix/{configs,shaders,res}, plus .desktop + icons
# main.cpp looks for the data relative to the executable using the same layout.
if(APPLE)
    set(ATOMIX_BIN_DIR .)
    set(ATOMIX_DATA_DIR atomix.app/Contents/Resources)
elseif(WIN32)
    set(ATOMIX_BIN_DIR .)
    set(ATOMIX_DATA_DIR .)
else()
    set(ATOMIX_BIN_DIR ${CMAKE_INSTALL_BINDIR})
    set(ATOMIX_DATA_DIR ${CMAKE_INSTALL_DATADIR}/atomix)
endif()

install(TARGETS atomix
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${ATOMIX_BIN_DIR}
)
install(DIRECTORY
    "${PROJECT_SOURCE_DIR}/configs"
    "${PROJECT_SOURCE_DIR}/shaders"
    "${PROJECT_SOURCE_DIR}/res"
    DESTINATION ${ATOMIX_DATA_DIR}
)

if(UNIX AND NOT APPLE)
    install(FILES "${PROJECT_SOURCE_DIR}/packaging/linux/atomix.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    foreach(size 256 512)
        install(FILES "${PROJECT_SOURCE_DIR}/res/icons/atomix${size}.png"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME atomix.png)
    endforeach()
    # Qt libraries and plugins are bundled afterwards by packaging/linux/build-appimage.sh (linuxdeploy).
endif()

if(APPLE)
    # Vulkan on macOS = Khronos loader (from vcpkg) + MoltenVK driver, both shipped inside the bundle.
    # The loader finds the driver through Contents/Resources/vulkan/icd.d; main.cpp points Qt at the loader.
    set(ATOMIX_MOLTENVK_DIR "" CACHE PATH "Directory containing libMoltenVK.dylib (e.g. from a MoltenVK release)")
    find_library(MOLTENVK_LIBRARY NAMES MoltenVK
        HINTS "${ATOMIX_MOLTENVK_DIR}" "$ENV{VULKAN_SDK}/lib" /opt/homebrew/lib /usr/local/lib
        NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
    if(NOT MOLTENVK_LIBRARY)
        message(FATAL_ERROR "libMoltenVK.dylib not found. Set ATOMIX_MOLTENVK_DIR to the directory containing it.")
    endif()
    file(REAL_PATH "${Vulkan_LIBRARY}" ATOMIX_VULKAN_LOADER)
    message(STATUS "Bundling Vulkan loader: ${ATOMIX_VULKAN_LOADER}")
    message(STATUS "Bundling MoltenVK:      ${MOLTENVK_LIBRARY}")

    set(_frameworks atomix.app/Contents/Frameworks)
    install(FILES "${ATOMIX_VULKAN_LOADER}" DESTINATION ${_frameworks} RENAME libvulkan.1.dylib)
    install(FILES "${MOLTENVK_LIBRARY}"     DESTINATION ${_frameworks} RENAME libMoltenVK.dylib)

    # library_path is relative to the JSON file: icd.d -> vulkan -> Resources -> Contents
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/MoltenVK_icd.json" [=[
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "../../../Frameworks/libMoltenVK.dylib",
        "api_version": "1.4.0",
        "is_portability_driver": true
    }
}
]=])
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MoltenVK_icd.json"
        DESTINATION atomix.app/Contents/Resources/vulkan/icd.d)
endif()

if(WIN32)
    # MSVC runtime DLLs next to the exe so it runs without the VC++ redistributable.
    # vulkan-1.dll is deliberately NOT shipped: every Windows GPU driver installs it.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${ATOMIX_BIN_DIR})
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
endif()

if(APPLE OR WIN32)
    # Runs macdeployqt / windeployqt against the installed app (Qt frameworks/DLLs + plugins).
    set(_deploy_opts "")
    if(WIN32)
        set(_deploy_opts --no-compiler-runtime --no-opengl-sw)
    endif()
    qt_generate_deploy_app_script(
        TARGET atomix
        OUTPUT_SCRIPT ATOMIX_DEPLOY_SCRIPT
        NO_TRANSLATIONS
        DEPLOY_TOOL_OPTIONS ${_deploy_opts}
    )
    install(SCRIPT "${ATOMIX_DEPLOY_SCRIPT}")
endif()
