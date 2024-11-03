set(CMAKE_AUTOMOC ON)
set(Qt6_DIR "/home/braer/Qt/6.5.3/gcc_64/lib/cmake/Qt6")

set(USING_QVULKAN 1)

find_package(TBB REQUIRED COMPONENTS tbb)
# set_source_files_properties(cloudmanager.cpp PROPERTIES COMPILE_OPTIONS "-O2")

if(DEFINED USING_QVULKAN)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent)
    find_package(glslang CONFIG REQUIRED)
    add_executable(atomix main.cpp quaternion.cpp shaderobj.cpp mainwindow.cpp configparser.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programVK.cpp vkwindow.cpp)
    add_compile_definitions(USING_QVULKAN)
    target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Concurrent tbb glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits TBB::tbb)
elseif(DEFINED USING_QOPENGL)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Concurrent)
    add_executable(atomix main.cpp quaternion.cpp shaderobj.cpp mainwindow.cpp configparser.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp programGL.cpp glwidget.cpp)
    add_compile_definitions(USING_QOPENGL)
    target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent tbb)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
