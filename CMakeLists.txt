cmake_minimum_required(VERSION 3.6)
project(cpd-game-of-life3d)

find_package(MPI REQUIRED)
include_directories(/usr/include/mpi/openmpi)

set(OMP_LINK_FLAGSET "-g")
set(OMP_LINK_FLAGSET2 "-o")
set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpic++)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${OMP_LINK_FLAGSET} ${OMP_LINK_FLAGSET2}" )

set(SOURCE_FILES life3d-mpi.cpp)
add_executable(cpd-game-of-life3d ${SOURCE_FILES})