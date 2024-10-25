set(CMAKE_AUTOMOC ON)
set(CMAKE_PREFIX_PATH "/usr/local/Qt-6.5.3")
# set(Qt6_DIR "~/Qt/6.5.3/macos")
set(Qt6_DIR "/usr/local/Qt-6.5.3/")
set(INCLUDE_BASE "/usr/local/include")
set(VULKAN_SDK "/Users/braer/VulkanSDK/1.3.296.0/macOS")
# set(GLM_INCLUDE_DIR "/usr/local/include/glm")
# set(TBB_INCLUDE_DIR "/usr/local/include/oneapi")

set(DYLD_LIBRARY_PATH ${VULKAN_SDK}/lib ${DYLD_LIBRARY_PATH})
set(VK_ADD_LAYER_PATH ${VULKAN_SDK}/share/vulkan/explicit_layer.d)
set(VK_ICD_FILENAMES ${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)
set(VK_DRIVER_FILES ${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Concurrent Gui)
find_package(oneDPL REQUIRED oneDPL)
find_package(TBB REQUIRED COMPONENTS tbb tbbmalloc)
find_package(Vulkan REQUIRED)

add_executable(atomix main.cpp quaternion.cpp shaderobj.cpp programGL.cpp mainwindow.cpp glwidget.cpp vkwindow.cpp configparser.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp)
# set_source_files_properties(cloudmanager.cpp PROPERTIES COMPILE_OPTIONS "-O2")

target_include_directories(atomix PRIVATE ${INCLUDE_BASE} ${VULKAN_SDK} ${Qt6_DIR})
target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent Qt6::Gui oneDPL TBB::tbb TBB::tbbmalloc Vulkan)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
