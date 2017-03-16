cmake_minimum_required(VERSION 3.6)
project(life3d)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp Cell.cpp Cell.h Generation.cpp Generation.h)
add_executable(life3d ${SOURCE_FILES})