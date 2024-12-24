set(CMAKE_AUTOMOC ON)
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(UNIX AND NOT APPLE)
    set(Qt6_DIR "/home/braer/Qt/6.8.0/gcc_64")
    set(oneDPL_DIR "/opt/intel/oneapi/2025.0/lib/cmake/oneDPL")
elseif(APPLE)
    set(Qt6_DIR "/usr/local/Qt-6.8.0")
    set(oneDPL_DIR "/opt/intel/oneapi/2022.2.0/lib/cmake/oneDPL")
elseif(WIN32)
    set(Qt6_DIR "C:/Qt/6.8.1/msvc2022_64")
endif()

set(HOST_LIBS ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/uni-osx/share)
list(APPEND CMAKE_PREFIX_PATH ${HOST_LIBS})
list(APPEND CMAKE_PREFIX_PATH ${Qt6_DIR})

set(ENV{VSCODE_QT_SDK} ${Qt6_DIR})
set(ENV{VSCODE_QT_FOLDER} ${Qt6_DIR})
# set(ENV{VK_LOADER_DEBUG} "error,warn,layer,info")

message(STATUS "<------- USING Qt6_DIR: ${Qt6_DIR} ------->")
message(STATUS "<------- USING HOST_LIBS: ${HOST_LIBS} ------>")
message(STATUS "<------- USING VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET} ------->")
message(STATUS "<------- USING VCPKG_HOST_TRIPLET: ${VCPKG_HOST_TRIPLET} ------->")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent Qml)
find_package(oneDPL REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

qt_add_executable(atomix main.cpp quaternion.cpp special.cpp shaderobj.cpp mainwindow.cpp filehandler.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programVK.cpp vkwindow.cpp)

target_link_libraries(atomix PRIVATE glm::glm glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV SPIRV-Tools-static SPIRV-Tools-opt spirv-cross-core spirv-cross-reflect oneDPL TBB::tbb TBB::tbbmalloc Vulkan::Vulkan)
target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Concurrent Qt6::Qml)

# Release Build and Install
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build/install)
    
    if(UNIX AND NOT APPLE)
        message(STATUS "Build/Install for Linux")
        # Run custom install script invoking appimagetool and copying/editing files
        add_custom_command(TARGET atomix POST_BUILD
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}
            COMMAND cp ${CMAKE_SOURCE_DIR}/doc/deploy/nix* ${CMAKE_INSTALL_PREFIX}/../
            COMMAND chmod +x ${CMAKE_INSTALL_PREFIX}/../nixdeploy.sh
            COMMENT "Generating macOS deployment files for atomix"
        )
        install(CODE "execute_process(COMMAND ./nixdeploy.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build)")
    elseif(APPLE)
        message(STATUS "Build/Install for Apple")
        # Run QT's macdeployqt and copy additional files
        set(LIBDIR ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Frameworks)
        set(RESDIR ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources)
        set_property(TARGET atomix PROPERTY MACOSX_BUNDLE true)
        set_target_properties(atomix PROPERTIES BUILD_RPATH "$ORIGIN/../lib")
        set_target_properties(atomix PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks")
        add_custom_command(TARGET atomix POST_BUILD
            COMMAND mkdir -p ${RESDIR}/configs && cp -r ${CMAKE_SOURCE_DIR}/configs/* ${RESDIR}/configs/
            COMMAND mkdir -p ${RESDIR}/shaders && cp -r ${CMAKE_SOURCE_DIR}/shaders/* ${RESDIR}/shaders/
            COMMAND mkdir -p ${RESDIR}/res && cp -r ${CMAKE_SOURCE_DIR}/res/* ${RESDIR}/res/
            COMMAND cp ${CMAKE_SOURCE_DIR}/res/icons/atomix.icns ${RESDIR}/
            COMMAND mkdir -p ${LIBDIR} && cp ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/libvulkan.1.3.296.dylib ${LIBDIR}/
            COMMAND ln -s ${LIBDIR}/libvulkan.1.3.296.dylib ${LIBDIR}/libvulkan.1.dylib
            COMMAND ln -s ${LIBDIR}/libvulkan.1.3.296.dylib ${LIBDIR}/libvulkan.dylib
            COMMENT "Generating macOS deployment files for atomix"
        )
    elseif(WIN32)
        message(STATUS "Build/Install for Windows")
        set_property(TARGET atomix PROPERTY WIN32_EXECUTABLE true)
    endif()

    install(TARGETS atomix
        BUNDLE DESTINATION .
        RUNTIME DESTINATION .
    )

    qt_generate_deploy_app_script(
        TARGET atomix
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    
    if (APPLE OR WIN32)
        install(SCRIPT ${deploy_script})
    endif()
endif()


