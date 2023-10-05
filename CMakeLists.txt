cmake_minimum_required(VERSION 3.0.0)
project(atomix VERSION 0.1.0 LANGUAGES C CXX)
#set(QT_INC "/usr/local/Qt-6.5.3/include")
#set(QT_LIB "/usr/local/Qt-6.5.3/lib")
set(Qt6_DIR "/home/braer/Qt/6.5.3/gcc_64/lib/cmake/Qt6")

include(CTest)
enable_testing()

#include_directories(${QT_INC})
#link_directories(${QT_LIB})

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL)

add_executable(atomix main.cpp)

target_link_libraries(atomix PRIVATE Qt6::Core Qt6::Widgets Qt6::OpenGL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
