cmake_minimum_required(VERSION 3.0)
project(Serialization)

set(INCLUDE_DIR ./includes)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")
set(CMAKE_BUILD_TYPE release)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(${INCLUDE_DIR})
aux_source_directory(./includes SOURCE_DIR)
add_executable(main main.cpp ${SOURCE_DIR})