cmake_minimum_required(VERSION 3.6)
project(cpd_game_of_life3d)

set(OMP_LINK_FLAGSET "-fopenmp")
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        tests/run_tests.py
        tests/s150e10k.1000.out
        tests/s150e10k.in
        tests/s200e50k.1000.out
        tests/s200e50k.in
        tests/s20e400.500.out
        tests/s20e400.in
        tests/s500e300k.2000.out
        tests/s500e300k.in
        tests/s50e5k.300.out
        tests/s50e5k.in
        tests/s5e50.10.out
        tests/s5e50.in
        life3d-omp.cpp
        README.md)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${OMP_LINK_FLAGSET}" )
add_executable(cpd_game_of_life3d ${SOURCE_FILES} )