set(CMAKE_AUTOMOC ON)
set(CMAKE_BUILD_TYPE Debug)
set(Qt6_DIR "/home/braer/Qt/6.8.0/gcc_64")
set(oneDPL_DIR "/opt/intel/oneapi/2025.0/lib/cmake/oneDPL")
set(ENV{VSCODE_QT_SDK} ${Qt6_DIR})
set(ENV{VULKAN_SDK} "/home/braer/VulkanSDK/1.3.296.0")
set(ENV{VK_LOADER_DEBUG} "error,warn,layer,info")

find_package(oneDPL REQUIRED)
find_package(TBB REQUIRED COMPONENTS tbb tbbmalloc)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent)
find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)

set_source_files_properties(cloudmanager.cpp PROPERTIES COMPILE_OPTIONS "-O2")

add_executable(atomix main.cpp quaternion.cpp special.cpp shaderobj.cpp mainwindow.cpp configparser.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programVK.cpp vkwindow.cpp)

target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Concurrent tbb glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits spirv-cross-core spirv-cross-reflect TBB::tbb TBB::tbbmalloc oneDPL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
