cmake_minimum_required(VERSION 3.0.0...3.5)
project(atomix VERSION 0.1.0 LANGUAGES C CXX)
add_subdirectory(src)
target_compile_options(atomix PRIVATE -std=c++20 -ltbb)
