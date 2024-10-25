cmake_minimum_required(VERSION 3.0.0...3.5)
project(atomix VERSION 0.1.0 LANGUAGES CXX)
add_subdirectory(src)
target_compile_features(atomix PUBLIC cxx_std_23)
