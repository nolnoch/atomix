set(CMAKE_AUTOMOC ON)
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(LINUX 0)
set(MACOS 1)
set(WIN11 0)

if(LINUX)
    set(Qt6_DIR "/home/braer/Qt/6.8.0/gcc_64")
    set(oneDPL_DIR "/opt/intel/oneapi/2025.0/lib/cmake/oneDPL")
    set(VULKAN_SDK "/home/braer/VulkanSDK/1.3.296.0")
elseif(MACOS)
    set(Qt6_DIR "/usr/local/Qt-6.8.0")
    set(oneDPL_DIR "/opt/intel/oneapi/2022.2.0/lib/cmake/oneDPL")
    set(Qt6GuiTools_DIR ${Qt6_DIR}/lib/cmake/Qt6GuiTools)
    set(VULKAN_SDK "/Users/braer/VulkanSDK/1.3.296.0/macOS")
    set(DYLD_LIBRARY_PATH ${VULKAN_SDK}/lib ${DYLD_LIBRARY_PATH})

    set(VK_ADD_LAYER_PATH ${VULKAN_SDK}/share/vulkan/explicit_layer.d)
    set(VK_ICD_FILENAMES ${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
    set(VK_DRIVER_FILES ${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
elseif(WIN11)
    set(Qt6_DIR "C:/Qt/6.8.1/msvc2022_64")
    set(VULKAN_SDK "C:/VulkanSDK/1.3.296.0")
    list(APPEND CMAKE_PREFIX_PATH "C:/dev/vcpkg/installed/x64-windows/share")
endif()

set(ENV{Qt6_DIR} ${Qt6_DIR})
set(ENV{VSCODE_QT_SDK} ${Qt6_DIR})
set(ENV{VSCODE_QT_FOLDER} ${Qt6_DIR})
set(ENV{VULKAN_SDK} ${VULKAN_SDK})
set(ENV{VK_LOADER_DEBUG} "error,warn,layer,info")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent Qml)
find_package(oneDPL REQUIRED oneDPL)
find_package(TBB REQUIRED COMPONENTS tbb tbbmalloc)
find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(atomix main.cpp quaternion.cpp special.cpp shaderobj.cpp mainwindow.cpp filehandler.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programVK.cpp vkwindow.cpp)

target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Concurrent Qt6::Qml glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits spirv-cross-core spirv-cross-reflect oneDPL TBB::tbb TBB::tbbmalloc)

# message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
# message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
# message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
# message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
# message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")

# Release Build and Install
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/build/install)
    
    if(LINUX)
        set(CPACK_PROJECT_NAME ${PROJECT_NAME})
        set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
        include(CPack)
        # set_target_properties(atomix PROPERTIES BUILD_RPATH "$ORIGIN/../lib")
        set_target_properties(atomix PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        add_custom_command(TARGET atomix PRE_BUILD
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/configs && cp -r ${CMAKE_SOURCE_DIR}/configs ${CMAKE_INSTALL_PREFIX}/configs
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/shaders && cp -r ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_INSTALL_PREFIX}/shaders
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/res && cp -r ${CMAKE_SOURCE_DIR}/res ${CMAKE_INSTALL_PREFIX}/res
            COMMENT "Generating Linux deployment files for atomix"
        )
    elseif(MACOS)
        set_property(TARGET atomix PROPERTY MACOSX_BUNDLE true)
        # set_target_properties(atomix PROPERTIES BUILD_RPATH "@loader_path/../Frameworks")
        set_target_properties(atomix PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks")
        add_custom_command(TARGET atomix POST_BUILD
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/configs && cp -r ${CMAKE_SOURCE_DIR}/configs/* ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/configs/
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/shaders && cp -r ${CMAKE_SOURCE_DIR}/shaders/* ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/shaders/
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/res && cp -r ${CMAKE_SOURCE_DIR}/res/* ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/res/
            COMMAND cp ${CMAKE_SOURCE_DIR}/res/icons/atomix.icns ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Resources/
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Frameworks && cp ${VULKAN_SDK}/lib/libvulkan.dylib ${CMAKE_INSTALL_PREFIX}/atomix.app/Contents/Frameworks/
            COMMENT "Generating macOS deployment files for atomix"
        )
    elseif(WIN11)
        set_property(TARGET atomix PROPERTY WIN32_EXECUTABLE true)
        # set_target_properties(atomix PROPERTIES BUILD_RPATH "$ORIGIN/../lib")
        set_target_properties(atomix PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        add_custom_command(TARGET atomix PRE_BUILD
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/configs && cp -r ${CMAKE_SOURCE_DIR}/configs ${CMAKE_INSTALL_PREFIX}/configs
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/shaders && cp -r ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_INSTALL_PREFIX}/shaders
            COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/res && cp -r ${CMAKE_SOURCE_DIR}/res ${CMAKE_INSTALL_PREFIX}/res
            COMMENT "Generating Windows deployment files for atomix"
        )
    endif()

    install(TARGETS atomix
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    qt_generate_deploy_app_script(
        TARGET atomix
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    install(SCRIPT ${deploy_script})
    
endif()


