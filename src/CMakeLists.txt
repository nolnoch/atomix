set(CMAKE_AUTOMOC ON)
set(CMAKE_PREFIX_PATH "~/Qt/6.5.3/macos")
set(Qt6_DIR "~/Qt/6.5.3/macos/lib/cmake/Qt6")
# set(GLM_INCLUDE_DIR "/usr/local/include/glm")
set(TBB_INCLUDE_DIR "/usr/local/include/oneapi")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Concurrent)
find_package(oneDPL REQUIRED oneDPL)
find_package(TBB REQUIRED COMPONENTS tbb tbbmalloc)

add_executable(atomix main.cpp quaternion.cpp shaderobj.cpp programGL.cpp mainwindow.cpp glwidget.cpp configparser.cpp manager.cpp wavemanager.cpp cloudmanager.cpp slideswitch.cpp)
# set_source_files_properties(cloudmanager.cpp PROPERTIES COMPILE_OPTIONS "-O2")

target_include_directories(atomix PRIVATE ${TBB_INCLUDE_DIR})
target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent TBB::tbb TBB::tbbmalloc oneDPL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
