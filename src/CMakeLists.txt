set(CMAKE_AUTOMOC ON)
set(Qt6_DIR "/home/braer/Qt/6.5.3/gcc_64/lib/cmake/Qt6")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Concurrent)
find_package(TBB REQUIRED COMPONENTS tbb)

add_executable(atomix main.cpp quaternion.cpp shaderobj.cpp program.cpp mainwindow.cpp GWidget.cpp configparser.cpp wavemanager.cpp cloudmanager.cpp manager.cpp)
#set_source_files_properties(cloudmanager.cpp PROPERTIES COMPILE_OPTIONS "-O2")

target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent tbb)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
