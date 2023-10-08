cmake_minimum_required(VERSION 3.0.0...3.5)
project(atomix VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_AUTOMOC ON)
set(Qt6_DIR "/home/braer/Qt/6.5.3/gcc_64/lib/cmake/Qt6")

#find_package(OpenGL REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)

add_executable(atomix main.cpp MainWindow.cpp Window.cpp GWidget.cpp)

target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
