cmake_minimum_required(VERSION 3.10)
project(micpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS ON)

# Main target for the compressor
add_executable(micompress src/micompress.cpp)
target_include_directories(micompress PRIVATE include)
