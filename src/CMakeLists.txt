set(CMAKE_AUTOMOC ON)
message(STATUS "<------- CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE} ------>")

if(UNIX AND NOT APPLE)
    set(Qt6_DIR "/home/braer/Qt/6.8.0/gcc_64")
    set(oneDPL_DIR "/opt/intel/oneapi/2025.0/lib/cmake/oneDPL")
elseif(APPLE)
    set(Qt6_DIR "/usr/local/Qt-6.8.0")
    set(oneDPL_DIR "/opt/intel/oneapi/2022.2.0/lib/cmake/oneDPL")
elseif(WIN32)
    set(Qt6_DIR "C:/Qt/6.8.1/msvc2022_64")
endif()

set(VC_LIBS ${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share)
list(APPEND CMAKE_PREFIX_PATH ${VC_LIBS})
list(APPEND CMAKE_PREFIX_PATH ${Qt6_DIR})

set(ENV{VSCODE_QT_SDK} ${Qt6_DIR})
set(ENV{VSCODE_QT_FOLDER} ${Qt6_DIR})

message(STATUS "<------- USING VC_LIBS: ${VC_LIBS} ------>")
message(STATUS "<------- USING Qt6_DIR: ${Qt6_DIR} ------->")
message(STATUS "<------- USING VCPKG_HOST_TRIPLET: ${VCPKG_HOST_TRIPLET} ------->")
message(STATUS "<------- USING VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET} ------->")

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

# Configure Install subroutines
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build/install)

if(APPLE)
    message(STATUS "Build/Install for Apple")
    # Run custom install script invoking macdeployqt and copying/editing files
    set_target_properties(atomix PROPERTIES
        MACOSX_BUNDLE true
        BUILD_RPATH "@loader_path/../lib"
        INSTALL_RPATH "@loader_path/../lib"
    )
    add_custom_command(TARGET atomix POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
        COMMAND cp ${CMAKE_SOURCE_DIR}/doc/deploy/macdeploy.sh deploy.sh
        COMMAND chmod +x deploy.sh
    )
elseif(UNIX)
    message(STATUS "Build/Install for Linux")
    # Run custom install script invoking appimagetool and copying/editing files
    add_custom_command(TARGET atomix POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
        COMMAND cp ${CMAKE_SOURCE_DIR}/doc/deploy/nixdeploy.sh deploy.sh
        COMMAND chmod +x deploy.sh
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

if(WIN32)
    install(SCRIPT ${deploy_script})
else()
    install(CODE "execute_process(COMMAND ./deploy.sh ${CMAKE_BUILD_TYPE} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build)")
endif()


